/*

Module:  catena4612_simple.ino

Function:
        Sensor program for Catena 4612 and variants.

Copyright notice:
        This file copyright (C) 2019 by

                MCCI Corporation
                3520 Krums Corners Road
                Ithaca, NY  14850

        See project LICENSE file for license information.

Author:
        Terry Moore, MCCI Corporation	February 2019

Revision history:
        See https://github.com/mcci-catena/Catena-Sketches

*/

#include <Catena.h>

#include <Catena_Led.h>
#include <Catena_TxBuffer.h>
#include <Catena_CommandStream.h>
#include <Catena_Mx25v8035f.h>

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Arduino_LoRaWAN.h>
#include <Catena_Si1133.h>
#include <lmic.h>
#include <hal/hal.h>
#include <mcciadk_baselib.h>

#include <cmath>
#include <type_traits>

using namespace McciCatena;

/****************************************************************************\
|
|	MANIFEST CONSTANTS & TYPEDEFS
|
\****************************************************************************/

constexpr uint8_t kUplinkPort = 2;

enum class FlagsSensorPort2 : uint8_t
        {
        FlagVbat = 1 << 0,
        FlagVcc = 1 << 1,
        FlagBoot = 1 << 2,
        FlagTPH = 1 << 3,	// temperature, pressure, humidity
        FlagLight = 1 << 4,	// Si1133 "ir", "white", "uv"
        FlagVbus = 1 << 5,	// Vbus input
        };

constexpr FlagsSensorPort2 operator| (const FlagsSensorPort2 lhs, const FlagsSensorPort2 rhs)
        {
        return FlagsSensorPort2(uint8_t(lhs) | uint8_t(rhs));
        };

FlagsSensorPort2 operator|= (FlagsSensorPort2 &lhs, const FlagsSensorPort2 &rhs)
        {
        lhs = lhs | rhs;
        return lhs;
        };

/* adjustable timing parameters */
enum    {
        // set this to interval between transmissions, in seconds
        // Actual time will be a little longer because have to
        // add measurement and broadcast time, but we attempt
        // to compensate for the gross effects below.
        CATCFG_T_CYCLE = 6 * 60,        // every 6 minutes
        CATCFG_T_CYCLE_TEST = 30,       // every 30 seconds
        CATCFG_T_CYCLE_INITIAL = 30,    // every 30 seconds initially
        CATCFG_INTERVAL_COUNT_INITIAL = 10,     // repeat for 5 minutes
        CATCFG_T_REBOOT = 30 * 24 * 60 * 60,    // reboot every 30 days
        };

/* additional timing parameters; ususually you don't change these. */
enum    {
        CATCFG_T_WARMUP = 1,
        CATCFG_T_SETTLE = 5,
        CATCFG_T_OVERHEAD = (CATCFG_T_WARMUP + CATCFG_T_SETTLE + 4),
        CATCFG_T_MIN = CATCFG_T_OVERHEAD,
        CATCFG_T_MAX = CATCFG_T_CYCLE < 60 * 60 ? 60 * 60 : CATCFG_T_CYCLE,     // normally one hour max.
        CATCFG_INTERVAL_COUNT = 30,
        };

constexpr uint32_t CATCFG_GetInterval(uint32_t tCycle)
        {
        return (tCycle < CATCFG_T_OVERHEAD + 1)
                ? 1
                : tCycle - CATCFG_T_OVERHEAD
                ;
        }

enum    {
        CATCFG_T_INTERVAL = CATCFG_GetInterval(CATCFG_T_CYCLE),
        };

// forwards
static void settleDoneCb(osjob_t *pSendJob);
static void warmupDoneCb(osjob_t *pSendJob);
static void txNotProvisionedCb(osjob_t *pSendJob);
static void sleepDoneCb(osjob_t *pSendJob);
static Arduino_LoRaWAN::SendBufferCbFn sendBufferDoneCb;
static Arduino_LoRaWAN::ReceivePortBufferCbFn receiveMessage;

/****************************************************************************\
|
|   handy constexpr to extract the base name of a file
|
\****************************************************************************/

// two-argument version: first arg is what to return if we don't find
// a directory separator in the second part.
static constexpr const char *filebasename(const char *s, const char *p)
    {
    return p[0] == '\0'                     ? s                            :
           (p[0] == '/' || p[0] == '\\')    ? filebasename(p + 1, p + 1)   :
                                              filebasename(s, p + 1)       ;
    }

static constexpr const char *filebasename(const char *s)
    {
    return filebasename(s, s);
    }


/****************************************************************************\
|
|	READ-ONLY DATA
|
\****************************************************************************/

static const char sVersion[] = "0.2.0";

/****************************************************************************\
|
|	VARIABLES
|
\****************************************************************************/

// the primary object
Catena gCatena;

//
// the LoRaWAN backhaul.  Note that we use the
// Catena version so it can provide hardware-specific
// information to the base class.
//
Catena::LoRaWAN gLoRaWAN;

//
// the LED
//
StatusLed gLed (Catena::PIN_STATUS_LED);

//   The temperature/humidity sensor
Adafruit_BME280 gBME280; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
Catena_Si1133 gSi1133;
bool fLight;

SPIClass gSPI2(
                Catena::PIN_SPI2_MOSI,
                Catena::PIN_SPI2_MISO,
                Catena::PIN_SPI2_SCK
                );

//   The flash
Catena_Mx25v8035f gFlash;
bool fFlash;

//  USB power
bool fUsbPower;

// have we printed the sleep info?
bool g_fPrintedSleeping = false;

//  the job that's used to synchronize us with the LMIC code
static osjob_t sensorJob;
void sensorJob_cb(osjob_t *pJob);

// the cycle time to use
unsigned gTxCycle;
// remaining before we reset to default
unsigned gTxCycleCount;



/*

Name:	setup()

Function:
        Arduino setup function.

Definition:
        void setup(
            void
            );

Description:
        This function is called by the Arduino framework after
        basic framework has been initialized. We initialize the sensors
        that are present on the platform, set up the LoRaWAN connection,
        and (ultimately) return to the framework, which then calls loop()
        forever.

Returns:
        No explicit result.

*/

void setup(void)
        {
        gCatena.begin();

        setup_platform();
        setup_light();
        setup_bme280();
        setup_flash();
        setup_uplink();
        }

void setup_platform(void)
        {
#ifdef USBCON
        // if running unattended, don't wait for USB connect.
        if (! (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fUnattended)))
                {
                while (!Serial)
                        /* wait for USB attach */
                        yield();
                }
#endif

        gCatena.SafePrintf("\n");
        gCatena.SafePrintf("-------------------------------------------------------------------------------\n");
        gCatena.SafePrintf("This is %s V%s.\n", filebasename(__FILE__), sVersion);
                {
                char sRegion[16];
                gCatena.SafePrintf("Target network: %s / %s\n",
                                gLoRaWAN.GetNetworkName(),
                                gLoRaWAN.GetRegionString(sRegion, sizeof(sRegion))
                                );
                }
        gCatena.SafePrintf("Enter 'help' for a list of commands.\n");
        gCatena.SafePrintf("(remember to select 'Line Ending: Newline' at the bottom of the monitor window.)\n");

        gCatena.SafePrintf("SYSCLK: %u MHz\n", unsigned(gCatena.GetSystemClockRate() / (1000 * 1000)));

#ifdef USBCON
        gCatena.SafePrintf("USB enabled\n");
#else
        gCatena.SafePrintf("USB disabled\n");
#endif

        gLoRaWAN.SetReceiveBufferBufferCb(receiveMessage);
        setTxCycleTime(CATCFG_T_CYCLE_INITIAL, CATCFG_INTERVAL_COUNT_INITIAL);

        Catena::UniqueID_string_t CpuIDstring;

        gCatena.SafePrintf(
                "CPU Unique ID: %s\n",
                gCatena.GetUniqueIDstring(&CpuIDstring)
                );

        gCatena.SafePrintf("--------------------------------------------------------------------------------\n");
        gCatena.SafePrintf("\n");

        // set up the LED
        gLed.begin();
        gCatena.registerObject(&gLed);
        gLed.Set(LedPattern::FastFlash);

        // set up LoRaWAN
        gCatena.SafePrintf("LoRaWAN init: ");
        if (!gLoRaWAN.begin(&gCatena))
                {
                gCatena.SafePrintf("failed\n");
                }
        else
                {
                gCatena.SafePrintf("succeeded\n");
                }

        gCatena.registerObject(&gLoRaWAN);

        /* find the platform */
        const Catena::EUI64_buffer_t *pSysEUI = gCatena.GetSysEUI();

        uint32_t flags;
        const CATENA_PLATFORM * const pPlatform = gCatena.GetPlatform();

        if (pPlatform)
                {
                gCatena.SafePrintf("EUI64: ");
                for (unsigned i = 0; i < sizeof(pSysEUI->b); ++i)
                        {
                        gCatena.SafePrintf("%s%02x", i == 0 ? "" : "-", pSysEUI->b[i]);
                        }
                gCatena.SafePrintf("\n");
                flags = gCatena.GetPlatformFlags();
                gCatena.SafePrintf(
                        "Platform Flags:  %#010x\n",
                        flags
                        );
                gCatena.SafePrintf(
                        "Operating Flags:  %#010x\n",
                        gCatena.GetOperatingFlags()
                        );
                }
        else
                {
                gCatena.SafePrintf("**** no platform, check provisioning ****\n");
                flags = 0;
                }
        }

void setup_light(void)
        {
        if (gSi1133.begin())
                {
                fLight = true;
                gSi1133.configure(0, CATENA_SI1133_MODE_SmallIR);
                gSi1133.configure(1, CATENA_SI1133_MODE_White);
                gSi1133.configure(2, CATENA_SI1133_MODE_UV);
                }
        else
                {
                fLight = false;
                gCatena.SafePrintf("No Si1133 found: check hardware\n");
                }
        }

void setup_bme280(void)
        {
        if (gBME280.begin(BME280_ADDRESS, Adafruit_BME280::OPERATING_MODE::Sleep))
                {
                fBme = true;
                }
        else
                {
                fBme = false;
                gCatena.SafePrintf("No BME280 found: check hardware\n");
                }
        }

void setup_flash(void)
        {
        if (gFlash.begin(&gSPI2, Catena::PIN_SPI2_FLASH_SS))
                {
                fFlash = true;
                gFlash.powerDown();
                gCatena.SafePrintf("FLASH found, put power down\n");
                }
        else
                {
                fFlash = false;
                gFlash.end();
                gSPI2.end();
                gCatena.SafePrintf("No FLASH found: check hardware\n");
                }
        }

uint32_t gRebootMs;

void setup_uplink(void)
        {
        LMIC_setClockError(1*65536/100);

        /* figure out when to reboot */
        gRebootMs = (CATCFG_T_REBOOT + os_getRndU2() - 32768) * 1000;

        /* trigger a join by sending the first packet */
        if (!(gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest)))
                {
                if (! gLoRaWAN.IsProvisioned())
                        gCatena.SafePrintf("LoRaWAN not provisioned yet. Use the commands to set it up.\n");
                else
                        {
                        gLed.Set(LedPattern::Joining);

                        /* warm up the BME280 by discarding a measurement */
                        if (fBme)
                                (void)gBME280.readTemperature();

                        /* trigger a join by sending the first packet */
                        startSendingUplink();
                        }
                }
        }

// The Arduino loop routine -- in our case, we just drive the other loops.
// If we try to do too much, we can break the LMIC radio. So the work is
// done by outcalls scheduled from the LMIC os loop.
void fillBuffer(TxBuffer_t &b);

void loop()
        {
        gCatena.poll();

        /* for mfg test, don't tx, just fill -- this causes output to Serial */
        if (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest))
                {
                TxBuffer_t b;
                fillBuffer(b);
                delay(1000);
                // since the light sensor was stopped in fillbuffer, restart it.
                }
        }

void fillBuffer(TxBuffer_t &b)
        {
        if (fLight)
                gSi1133.start(true);

        b.begin();
        FlagsSensorPort2 flag;

        flag = FlagsSensorPort2(0);

        // we have no format byte for this.
        uint8_t * const pFlag = b.getp();
        b.put(0x00); /* will be set to the flags */

        // vBat is sent as 5000 * v
        float vBat = gCatena.ReadVbat();
        gCatena.SafePrintf("vBat:    %d mV\n", (int) (vBat * 1000.0f));
        b.putV(vBat);
        flag |= FlagsSensorPort2::FlagVbat;

        // vBus is sent as 5000 * v
        float vBus = gCatena.ReadVbus();
        gCatena.SafePrintf("vBus:    %d mV\n", (int) (vBus * 1000.0f));
        fUsbPower = (vBus > 3.0) ? true : false;

        uint32_t bootCount;
        if (gCatena.getBootCount(bootCount))
                {
                b.putBootCountLsb(bootCount);
                flag |= FlagsSensorPort2::FlagBoot;
                }

        if (fBme)
                {
                Adafruit_BME280::Measurements m = gBME280.readTemperaturePressureHumidity();
                // temperature is 2 bytes from -0x80.00 to +0x7F.FF degrees C
                // pressure is 2 bytes, hPa * 10.
                // humidity is one byte, where 0 == 0/256 and 0xFF == 255/256.
                gCatena.SafePrintf(
                        "BME280:  T: %d P: %d RH: %d\n",
                        (int) m.Temperature,
                        (int) m.Pressure,
                        (int) m.Humidity
                        );
                b.putT(m.Temperature);
                b.putP(m.Pressure);
                b.putRH(m.Humidity);

                flag |= FlagsSensorPort2::FlagTPH;
                }

        if (fLight)
                {
                /* Get a new sensor event */
                uint16_t data[3];

                while (! gSi1133.isOneTimeReady())
                        {
                        yield();
                        }

                gSi1133.readMultiChannelData(data, 3);
                gSi1133.stop();
                gCatena.SafePrintf(
                        "Si1133:  %u IR, %u White, %u UV\n",
                        data[0],
                        data[1],
                        data[2]
                        );

                b.putLux(data[0]);
                b.putLux(data[1]);
                b.putLux(data[2]);

                flag |= FlagsSensorPort2::FlagLight;
                }

        b.putV(vBus);
        flag |= FlagsSensorPort2::FlagVbus;

        *pFlag = uint8_t(flag);
        }

void startSendingUplink(void)
        {
        TxBuffer_t b;
        LedPattern savedLed = gLed.Set(LedPattern::Measuring);

        fillBuffer(b);
        if (savedLed != LedPattern::Joining)
                gLed.Set(LedPattern::Sending);
        else
                gLed.Set(LedPattern::Joining);

        bool fConfirmed = false;
        if (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fConfirmedUplink))
                {
                gCatena.SafePrintf("requesting confirmed tx\n");
                fConfirmed = true;
                }

        gLoRaWAN.SendBuffer(b.getbase(), b.getn(), sendBufferDoneCb, NULL, fConfirmed, kUplinkPort);
        }

static void sendBufferDoneCb(
        void *pContext,
        bool fStatus
        )
        {
        osjobcb_t pFn;

        gLed.Set(LedPattern::Settling);

        pFn = settleDoneCb;
        if (! fStatus)
                {
                if (!gLoRaWAN.IsProvisioned())
                        {
                        // we'll talk about it at the callback.
                        pFn = txNotProvisionedCb;

                        // but prevent join attempts now.
                        gLoRaWAN.Shutdown();
                        }
                else
                        gCatena.SafePrintf("send buffer failed\n");
                }

        os_setTimedCallback(
                &sensorJob,
                os_getTime()+sec2osticks(CATCFG_T_SETTLE),
                pFn
                );
        }

static void txNotProvisionedCb(
        osjob_t *pSendJob
        )
        {
        gCatena.SafePrintf("LoRaWAN not provisioned yet. Use the commands to set it up.\n");
        gLoRaWAN.Shutdown();
        gLed.Set(LedPattern::NotProvisioned);
        }


static void settleDoneCb(
        osjob_t *pSendJob
        )
        {
        const bool fDeepSleep = checkDeepSleep();

        if (uint32_t(millis()) > gRebootMs)
                {
                // time to reboot
                NVIC_SystemReset();
                }

        if (! g_fPrintedSleeping)
                doSleepAlert(fDeepSleep);

        /* count what we're up to */
        updateSleepCounters();

        if (fDeepSleep)
                doDeepSleep(pSendJob);
        else
                doLightSleep(pSendJob);
        }

bool checkDeepSleep(void)
        {
        bool const fDeepSleepTest = gCatena.GetOperatingFlags() &
                        static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fDeepSleepTest);
        bool fDeepSleep;

        if (fDeepSleepTest)
                {
                fDeepSleep = true;
                }
#ifdef USBCON
        else if (Serial.dtr())
                {
                fDeepSleep = false;
                }
#endif
        else if (gCatena.GetOperatingFlags() &
                        static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fDisableDeepSleep))
                {
                fDeepSleep = false;
                }
        else if ((gCatena.GetOperatingFlags() &
                        static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fUnattended)) != 0)
                {
                fDeepSleep = true;
                }
        else
                {
                fDeepSleep = false;
                }

        return fDeepSleep;
        }

void doSleepAlert(const bool fDeepSleep)
        {
        g_fPrintedSleeping = true;

        if (fDeepSleep)
                {
                bool const fDeepSleepTest = gCatena.GetOperatingFlags() &
                                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fDeepSleepTest);
                const uint32_t deepSleepDelay = fDeepSleepTest ? 10 : 30;

                gCatena.SafePrintf("using deep sleep in %u secs"
#ifdef USBCON
                                   " (USB will disconnect while asleep)"
#endif
                                   ": ",
                                        deepSleepDelay
                                        );

                // sleep and print
                gLed.Set(LedPattern::TwoShort);

                for (auto n = deepSleepDelay; n > 0; --n)
                        {
                        uint32_t tNow = millis();

                        while (uint32_t(millis() - tNow) < 1000)
                                {
                                gCatena.poll();
                                yield();
                                }
                        gCatena.SafePrintf(".");
                        }
                gCatena.SafePrintf("\nStarting deep sleep.\n");
                uint32_t tNow = millis();
                while (uint32_t(millis() - tNow) < 100)
                        {
                        gCatena.poll();
                        yield();
                        }
                }
        else
                gCatena.SafePrintf("using light sleep\n");
        }

void updateSleepCounters(void)
        {
        // update the sleep parameters
        if (gTxCycleCount > 1)
                {
                // values greater than one are decremented and ultimately reset to default.
                --gTxCycleCount;
                }
        else if (gTxCycleCount == 1)
                {
                // it's now one (otherwise we couldn't be here.)
                gCatena.SafePrintf("resetting tx cycle to default: %u\n", CATCFG_T_CYCLE);

                gTxCycleCount = 0;
                gTxCycle = CATCFG_T_CYCLE;
                }
        else
                {
                // it's zero. Leave it alone.
                }
        }

void doDeepSleep(osjob_t *pJob)
        {
        bool const fDeepSleepTest = gCatena.GetOperatingFlags() &
                                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fDeepSleepTest);
        uint32_t const sleepInterval = CATCFG_GetInterval(
                        fDeepSleepTest ? CATCFG_T_CYCLE_TEST : gTxCycle
                        );

        /* ok... now it's time for a deep sleep */
        gLed.Set(LedPattern::Off);
        deepSleepPrepare();

        /* sleep */
        gCatena.Sleep(sleepInterval);

        /* recover from sleep */
        deepSleepRecovery();

        /* and now... we're awake again. trigger another measurement */
        sleepDoneCb(pJob);
        }

void deepSleepPrepare(void)
        {
        Serial.end();
        Wire.end();
        SPI.end();
        if (fFlash)
                gSPI2.end();
        }

void deepSleepRecovery(void)
        {
        Serial.begin();
        Wire.begin();
        SPI.begin();
        if (fFlash)
                gSPI2.begin();
        }

void doLightSleep(osjob_t *pJob)
        {
        uint32_t interval = sec2osticks(CATCFG_GetInterval(gTxCycle));

        gLed.Set(LedPattern::Sleeping);

        if (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fQuickLightSleep))
                {
                interval = 1;
                }

        gLed.Set(LedPattern::Sleeping);
        os_setTimedCallback(
                &sensorJob,
                os_getTime() + interval,
                sleepDoneCb
                );
        }

static void sleepDoneCb(
        osjob_t *pJob
        )
        {
        gLed.Set(LedPattern::WarmingUp);

        os_setTimedCallback(
                &sensorJob,
                os_getTime() + sec2osticks(CATCFG_T_WARMUP),
                warmupDoneCb
                );
        }

static void warmupDoneCb(
        osjob_t *pJob
        )
        {
        startSendingUplink();
        }

static void receiveMessage(
        void *pContext,
        uint8_t port,
        const uint8_t *pMessage,
        size_t nMessage
        )
        {
        unsigned txCycle;
        unsigned txCount;

        if (port == 0)
                {
                gCatena.SafePrintf("MAC message:");
                for (unsigned i = 0; i < LMIC.dataBeg; ++i)
                        {
                        gCatena.SafePrintf(" %02x", LMIC.frame[i]);
                        }
                gCatena.SafePrintf("\n");
                return;
                }

        else if (! (port == 1 && 2 <= nMessage && nMessage <= 3))
                {
                gCatena.SafePrintf("invalid message port(%02x)/length(%x)\n",
                        port, nMessage
                        );
                return;
                }

        txCycle = (pMessage[0] << 8) | pMessage[1];

        if (txCycle < CATCFG_T_MIN || txCycle > CATCFG_T_MAX)
                {
                gCatena.SafePrintf("tx cycle time out of range: %u\n", txCycle);
                return;
                }

        // byte [2], if present, is the repeat count.
        // explicitly sending zero causes it to stick.
        txCount = CATCFG_INTERVAL_COUNT;
        if (nMessage >= 3)
                {
                txCount = pMessage[2];
                }

        setTxCycleTime(txCycle, txCount);
        }

void setTxCycleTime(
        unsigned txCycle,
        unsigned txCount
        )
        {
        if (txCount > 0)
                gCatena.SafePrintf(
                        "message cycle time %u seconds for %u messages\n",
                        txCycle, txCount
                        );
        else
                gCatena.SafePrintf(
                        "message cycle time %u seconds indefinitely\n",
                        txCycle
                        );

        gTxCycle = txCycle;
        gTxCycleCount = txCount;
        }
