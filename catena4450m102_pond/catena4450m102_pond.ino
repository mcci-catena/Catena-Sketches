/*

Module:  catena4450m102_pond.ino

Function:
        Code for the pond sensor with Catena 4450-M102.

Copyright notice:
        See LICENSE file accompanying this project

Author:
        Terry Moore, MCCI Corporation	March 2017

*/

#include <Catena.h>

#ifdef ARDUINO_ARCH_SAMD
# include <CatenaRTC.h>
#elif defined(ARDUINO_ARCH_STM32)
# include <CatenaStm32L0Rtc.h>
#endif

#include <Catena_Led.h>
#include <Catena_TxBuffer.h>
#include <Catena_CommandStream.h>
#include <Catena_Totalizer.h>

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Arduino_LoRaWAN.h>
#include <BH1750.h>
#include <lmic.h>
#include <hal/hal.h>
#include <mcciadk_baselib.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SHT1x.h>

#include <cmath>
#include <type_traits>

/****************************************************************************\
|
|       Manifest constants & typedefs.
|
\****************************************************************************/

using namespace McciCatena;

/* how long do we wait between transmissions? (in seconds) */
enum    {
        // set this to interval between transmissions, in seconds
        // Actual time will be a little longer because have to
        // add measurement and broadcast time, but we attempt
        // to compensate for the gross effects below.
        CATCFG_T_CYCLE = 6 * 60,        // every 6 minutes
        CATCFG_T_CYCLE_TEST = 30,       // every 10 seconds
        };

/* additional timing parameters; ususually you don't change these. */
enum    {
        CATCFG_T_WARMUP = 1,
        CATCFG_T_SETTLE = 5,
        CATCFG_T_OVERHEAD = (CATCFG_T_WARMUP + CATCFG_T_SETTLE),
        };

constexpr uint32_t CATCFG_GetInterval(uint32_t tCycle)
        {
        return (tCycle < CATCFG_T_OVERHEAD)
                ? CATCFG_T_OVERHEAD
                : tCycle - CATCFG_T_OVERHEAD
                ;
        }

enum    {
        CATCFG_T_INTERVAL = CATCFG_GetInterval(CATCFG_T_CYCLE),
        };

enum    {
        PIN_ONE_WIRE =  A2,        // XSDA1 == A2
        PIN_SHT10_CLK = A1,        // XSCL0 == A1
        PIN_SHT10_DATA = A0,       // XSDA0 == A0
        };

// forwards
static void settleDoneCb(osjob_t *pSendJob);
static void warmupDoneCb(osjob_t *pSendJob);
static void txFailedDoneCb(osjob_t *pSendJob);
static void sleepDoneCb(osjob_t *pSendJob);
static Arduino_LoRaWAN::SendBufferCbFn sendBufferDoneCb;
void fillBuffer(TxBuffer_t &b);
void startSendingUplink(void);
void sensorJob_cb(osjob_t *pJob);


/****************************************************************************\
|
|       Read-only data.
|
\****************************************************************************/

static const char sVersion[] = "0.2.1";

/****************************************************************************\
|
|       Variables.
|
\****************************************************************************/

// the Catena instance
Catena gCatena;

//
// the LoRaWAN backhaul.  Note that we use the
// Catena version so it can provide hardware-specific
// information to the base class.
//
Catena::LoRaWAN gLoRaWAN;

// the LED instance object
StatusLed gLed (Catena::PIN_STATUS_LED);

// the RTC instance, used for sleeping
#ifdef ARDUINO_ARCH_SAMD
CatenaRTC gRtc;
#elif defined(ARDUINO_ARCH_STM32)
# include <CatenaStm32L0Rtc.h>
#endif /* ARDUINO_ARCH_SAMD*/

// The BME280 instance, for the temperature/humidity sensor
Adafruit_BME280 gBME280; // The default initalizer creates an I2C connection
bool fBme;               // true if BME280 is in use

//   The LUX sensor
BH1750 gBH1750;
bool fLux;              // true if BH1750 is in use

//   The submersible temperature sensor
OneWire oneWire(PIN_ONE_WIRE);
DallasTemperature sensor_WaterTemp(&oneWire);
bool fHasWaterTemp;
bool fFoundWaterTemp;

//  The SHT10 soil sensor
SHT1x sensor_Soil(PIN_SHT10_DATA, PIN_SHT10_CLK);
bool fSoilSensor;

//   The contact sensors
bool fHasPower1;
uint8_t kPinPower1P1;
uint8_t kPinPower1P2;

cTotalizer gPower1P1;
cTotalizer gPower1P2;

//  the LMIC job that's used to synchronize us with the LMIC code
static osjob_t sensorJob;

// debug flag for throttling sleep prints
bool g_fPrintedSleeping = false;

/****************************************************************************\
|
|       Code.
|
\****************************************************************************/

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
        setup_sensors();

        /* for 4551, we need wider tolerances, it seems */
#if defined(ARDUINO_ARCH_STM32)
        LMIC_setClockError(10 * 65536 / 100);
#endif

        /* trigger a join by sending the first packet */
        if (!(gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest)))
                {
                gLed.Set(LedPattern::Joining);

                /* warm up the BME280 by discarding a measurement */
                if (fBme)
                        (void)gBME280.readTemperature();

                /* trigger a join by sending the first packet */
                startSendingUplink();
                }
        }

void setup_platform()
        {
        // if running unattended, don't wait for USB connect.
        if (!(gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fUnattended)))
                {
                while (!Serial)
                        /* wait for USB attach */
                        yield();
                }

        gCatena.SafePrintf("\n");
        gCatena.SafePrintf("-------------------------------------------------------------------------------\n");
        gCatena.SafePrintf("This is the catena4450m102_pond program V%s.\n", sVersion);
                {
                char sRegion[16];
                gCatena.SafePrintf("Target network: %s / %s\n",
                                gLoRaWAN.GetNetworkName(),
                                gLoRaWAN.GetRegionString(sRegion, sizeof(sRegion))
                                );
                }
        gCatena.SafePrintf("Enter 'help' for a list of commands.\n");
        gCatena.SafePrintf("(remember to select 'Line Ending: Newline' at the bottom of the monitor window.)\n");
        gCatena.SafePrintf("--------------------------------------------------------------------------------\n");
        gCatena.SafePrintf("\n");

        // set up the LED
        gLed.begin();
        gCatena.registerObject(&gLed);
        gLed.Set(LedPattern::FastFlash);

        // set up the RTC object
        gRtc.begin();

        gCatena.SafePrintf("LoRaWAN init: ");
        if (!gLoRaWAN.begin(&gCatena))
                {
                gCatena.SafePrintf("failed\n");
                gCatena.registerObject(&gLoRaWAN);
                }
        else
                {
                gCatena.SafePrintf("succeeded\n");
                gCatena.registerObject(&gLoRaWAN);
                }

        // display the CPU unique ID
        Catena::UniqueID_string_t CpuIDstring;

        gCatena.SafePrintf("CPU Unique ID: %s\n",
                gCatena.GetUniqueIDstring(&CpuIDstring)
                );

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

        /* is it modded? */
        uint32_t modnumber = gCatena.PlatformFlags_GetModNumber(flags);

        fHasPower1 = false;

        if (modnumber != 0)
                {
                gCatena.SafePrintf("Catena 4450-M%u\n", modnumber);
                if (modnumber == 101)
                        {
                        fHasPower1 = true;
                        kPinPower1P1 = A0;
                        kPinPower1P2 = A1;
                        }
                else if (modnumber == 102 || modnumber == 103 || modnumber == 104)
                        {
                        fHasWaterTemp = flags & CatenaBase::fHasWaterOneWire;
                        fSoilSensor = flags & CatenaBase::fHasSoilProbe;
                        }
                else
                        {
                        gCatena.SafePrintf("unknown mod number %d\n", modnumber);
                        }
                }
        else
                {
                gCatena.SafePrintf("No mods detected\n");
                fHasWaterTemp = flags & CatenaBase::fHasWaterOneWire;
                fSoilSensor = flags & CatenaBase::fHasSoilProbe;
                }
        }

void setup_sensors(void)
        {
        uint32_t const flags = gCatena.GetPlatformFlags();

        /* initialize the lux sensor */
        if (flags & Catena::fHasLuxRohm)
                {
                gBH1750.begin();
                fLux = true;
                gBH1750.configure(BH1750_POWER_DOWN);
                }
        else
                {
                fLux = false;
                }

        /* initialize the BME280 */
        if (!gBME280.begin(BME280_ADDRESS, Adafruit_BME280::OPERATING_MODE::Sleep))
                {
                gCatena.SafePrintf("No BME280 found: defective board or incorrect platform\n");
                fBme = false;
                }
        else
                {
                fBme = true;
                }

        if (fHasWaterTemp)
                {
                sensor_WaterTemp.begin();
                if (sensor_WaterTemp.getDeviceCount() == 0)
                        {
                        gCatena.SafePrintf("No one-wire temperature sensor detected\n");
                        fFoundWaterTemp = false;
                        }
                else
                        fFoundWaterTemp = true;
                }
        }

// The Arduino loop routine -- in our case, we just drive the other loops.
// If we try to do too much, we can break the LMIC radio. So the work is
// done by outcalls scheduled from the LMIC os loop.
void loop()
        {
        gCatena.poll();

        /* for mfg test, don't tx, just fill */
        if (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest))
                {
                TxBuffer_t b;
                fillBuffer(b);
                delay(1000);
                }
        }

void fillBuffer(TxBuffer_t &b)
        {
        uint32_t tLuxBegin;

        /* start a Lux measurement */
        if (fLux)
                {
                gBH1750.configure(BH1750_ONE_TIME_HIGH_RES_MODE_2);
                tLuxBegin = millis();
                }
        else
                tLuxBegin = 0;

        /* meanwhile, fill the buffer */
        b.begin();
        FlagsSensor3 flag;

        flag = FlagsSensor3(0);

        b.put(FormatSensor3); /* the flag for this record format */
        uint8_t * const pFlag = b.getp();
        b.put(0x00); /* will be set to the flags */

        // vBat is sent as 5000 * v
        float vBat = gCatena.ReadVbat();
        gCatena.SafePrintf("vBat:    %d mV\n", (int)(vBat * 1000.0f));
        b.putV(vBat);
        flag |= FlagsSensor3::FlagVbat;

        uint32_t bootCount;
        if (gCatena.getBootCount(bootCount))
                {
                b.putBootCountLsb(bootCount);
                flag |= FlagsSensor3::FlagBoot;
                }

        if (fBme)
                {
                Adafruit_BME280::Measurements m = gBME280.readTemperaturePressureHumidity();
                // temperature is 2 bytes from -0x80.00 to +0x7F.FF degrees C
                // pressure is 2 bytes, hPa * 10.
                // humidity is one byte, where 0 == 0/256 and 0xFF == 255/256.
                gCatena.SafePrintf(
                        "BME280:  T: %d P: %d RH: %d\n",
                        (int)m.Temperature,
                        (int)m.Pressure,
                        (int)m.Humidity
                        );
                b.putT(m.Temperature);
                b.putP(m.Pressure);
                b.putRH(m.Humidity);

                flag |= FlagsSensor3::FlagTPH;
                }

        if (fLux)
                {
                /* Measure light. */
                uint16_t light;
                uint32_t tMeas = gBH1750.getMeasurementMillis();

                while (uint32_t(millis() - tLuxBegin) < tMeas)
                        {
                        gCatena.poll();
                        yield();
                        }

                light = gBH1750.readLightLevel();
                gCatena.SafePrintf("BH1750:  %u lux\n", light);
                b.putLux(light);
                flag |= FlagsSensor3::FlagLux;
                }


        if (fHasWaterTemp)
                {
                if (!fFoundWaterTemp)
                        {
                        sensor_WaterTemp.begin();
                        if (sensor_WaterTemp.getDeviceCount() != 0)
                                fFoundWaterTemp = true;
                        }
                if (fFoundWaterTemp)
                        {
                        sensor_WaterTemp.requestTemperatures();
                        float waterTempC = sensor_WaterTemp.getTempCByIndex(0);

                        gCatena.SafePrintf("Water:   T: %d\n", (int)waterTempC);
                        if (waterTempC < -55.0 || waterTempC > 125.0)
                                {
                                // discard data and reset flag
                                // so we'll check again next time
                                fFoundWaterTemp = false;
                                }
                        else
                                {
                                // transmit the measurement
                                b.putT(waterTempC);
                                flag |= FlagsSensor3::FlagWater;
                                }
                        }
                else
                        {
                        gCatena.SafePrintf("Water:   Sensor not responding\n");
                        }
                }

        /*
        || Measure and transmit the "soil sensor" transducer
        || values. The sensor can be hotplugged at will; the
        || library being used gets very slow if the sensor is
        || not being used, but otherwise adapts nicely.
        */
        if (fSoilSensor)
                {
                /* display temp and RH. library doesn't tell whether sensor is disconnected but gives us huge values instead */
                float SoilT = sensor_Soil.readTemperatureC();
                if (SoilT <= 100.0)
                        {
                        float SoilRH = sensor_Soil.readHumidity();

                        b.putT(SoilT);
                        b.putRH(SoilRH);
                        gCatena.SafePrintf("Soil:    T: %d RH: %d\n", (int)SoilT, (int)SoilRH);

                        flag |= FlagsSensor3::FlagSoilTH;
                        }
                else
                        {
                        gCatena.SafePrintf("Soil:    Sensor not responding\n");
                        }
                }

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
        if (gCatena.GetOperatingFlags() & (1 << 16))
                {
                gCatena.SafePrintf("requesting confirmed tx\n");
                fConfirmed = true;
                }

        gLoRaWAN.SendBuffer(b.getbase(), b.getn(), sendBufferDoneCb, NULL, fConfirmed);
        }

static void
sendBufferDoneCb(
        void *pContext,
        bool fStatus
        )
        {
        osjobcb_t pFn;

        gLed.Set(LedPattern::Settling);
        if (!fStatus)
                {
                gCatena.SafePrintf("send buffer failed\n");
                pFn = txFailedDoneCb;
                }
        else
                {
                pFn = settleDoneCb;
                }
        os_setTimedCallback(
                &sensorJob,
                os_getTime() + sec2osticks(CATCFG_T_SETTLE),
                pFn
                );
        }

static void
txFailedDoneCb(
        osjob_t *pSendJob
        )
        {
        gCatena.SafePrintf("not provisioned, idling\n");
        gLoRaWAN.Shutdown();
        gLed.Set(LedPattern::NotProvisioned);
        }


//
// the following API is added to delay.c, .h in the BSP. It adjusts millis()
// forward after a deep sleep.
//
// extern "C" { void adjust_millis_forward(unsigned); };
//
// If you don't have it, make sure you're running the MCCI Board Support
// Package for MCCI Catenas, https://github.com/mcci-catena/arduino-boards
//

static void settleDoneCb(
        osjob_t *pSendJob
        )
        {
        uint32_t startTime;
        bool const fDeepSleepTest = gCatena.GetOperatingFlags() & (1 << 19);
        bool fDeepSleep;

        if (fDeepSleepTest)
                {
                fDeepSleep = true;
                }
        else if (Serial.dtr() || fHasPower1 || (gCatena.GetOperatingFlags() & (1 << 17)))
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

        if (! g_fPrintedSleeping)
                {
                g_fPrintedSleeping = true;

                if (fDeepSleep)
                        {
                        const uint32_t deepSleepDelay = fDeepSleepTest ? 10 : 30;

                        gCatena.SafePrintf("using deep sleep in %u secs (USB will disconnect while asleep): ",
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


        // if connected to USB, don't sleep
        // ditto if we're monitoring pulses.
        if (! fDeepSleep)
                {
                uint32_t interval = sec2osticks(CATCFG_T_INTERVAL);

                if (gCatena.GetOperatingFlags() & (1 << 18))
                        interval = 1;

                gLed.Set(LedPattern::Sleeping);
                os_setTimedCallback(
                        &sensorJob,
                        os_getTime() + sec2osticks(CATCFG_T_INTERVAL),
                        sleepDoneCb
                        );
                return;
                }

        /*
        || ok... now it's time for a deep sleep.  Do the sleep here, since
        || the Arduino loop won't do it. Note that nothing will get polled
        || while we sleep. We can't poll if we're polling power.
        */
        gLed.Set(LedPattern::Off);
        USBDevice.detach();

        startTime = millis();
        uint32_t const sleepInterval = CATCFG_GetInterval(
                        fDeepSleepTest ? CATCFG_T_CYCLE_TEST : CATCFG_T_CYCLE
                        );

        gRtc.SetAlarm(sleepInterval);
        gRtc.SleepForAlarm(
                gRtc.MATCH_HHMMSS,
                // gRtc.SleepMode::IdleCpuAhbApb
                gRtc.SleepMode::DeepSleep
                );

        // add the number of ms that we were asleep to the millisecond timer.
        // we don't need extreme accuracy.
        adjust_millis_forward(sleepInterval * 1000);

        USBDevice.attach();

        /* and now... we're awake again. Go to next state. */
        sleepDoneCb(pSendJob);
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
