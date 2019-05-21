/*

Module:  catena4460_bsec_ulp.ino

Function:
        Code for the air-quality sensor with Catena 4460 using BSEC.

Copyright notice and License:
        See LICENSE file accompanying this project.

Author:
        Terry Moore, MCCI Corporation   March 2018

        Based on Bosch BSEC sample code, but substantially modified.

*/

#include <Catena4460.h>
#include <Catena_Led.h>
#include <Catena_TxBuffer.h>
#include <Catena_CommandStream.h>

#ifdef ARDUINO_ARCH_SAMD
# include <CatenaRTC.h>
#endif

#include "bsec.h"
#include <cstring>
#include "mcciadk_baselib.h"
#include <BH1750.h>
#include <lmic.h>
#include <hal/hal.h>


#include <cmath>
#include <type_traits>


/****************************************************************************\
|
|       Manifest constants & typedefs.
|
\****************************************************************************/

using namespace McciCatena;

constexpr uint32_t STATE_SAVE_PERIOD = UINT32_C(3 * 60 * 1000); // 3 minutes: basically any change.

/* how long do we wait between transmissions? (in seconds) */
enum	{
	// set this to interval between transmissions, in seconds
	// Actual time will be a little longer because have to
	// add measurement and broadcast time, but we attempt
	// to compensate for the gross effects below.
	CATCFG_T_CYCLE =  6 * 60,        // every 6 minutes
	CATCFG_T_CYCLE_TEST = 30,       // every 30 seconds
	CATCFG_T_CYCLE_INITIAL = 30,    // every 30 seconds initially
	CATCFG_INTERVAL_COUNT_INITIAL = 30,     // repeat for 15 minutes
	};

/* additional timing parameters; ususually you don't change these. */
enum	{
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

enum	{
	CATCFG_T_INTERVAL = CATCFG_GetInterval(CATCFG_T_CYCLE),
	};

// Helper function declarations
void checkIaqSensorStatus(void);
void errLeds(void);
void loadCalibrationData(void);
void possiblySaveCalibrationData(void);

// the Catena object
using Catena = Catena4460;

// other forwards
static void settleDoneCb(osjob_t *pSendJob);
static void warmupDoneCb(osjob_t *pSendJob);
static void txNotProvisionedCb(osjob_t *pSendJob);
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

static const char sVersion[] = "0.2.2";

extern const uint8_t bsec_config_iaq[];
//#include "bsec_serialized_configurations_iaq.h"

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
CatenaRTC gRtc;

// declare the Lux sensor instance
BH1750 gBH1750;

// Create an object of the class Bsec
Bsec iaqSensor;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;
uint32_t millisOverflowCounter = 0;
uint32_t lastTime = 0;

String output;

// time stamp of last time we saved the state.
uint32_t lastCalibrationWriteMillis = 0;

//  the LMIC job that's used to synchronize us with the LMIC code
static osjob_t sensorJob;

// measurements come out of BSEC as they well; we store the
// ones we want here for transmission at the designated
// transmit times.
bool g_dataValid;
bool g_iaqValid;
uint8_t g_iaqAccuracy = 0;
float g_temperature = 0.0;
float g_pressure = 0.0;
float g_humidity = 0.0;
float g_iaq = 0.0;
float g_gas_resistance = 0.0;

// debug flag for throttling sleep prints
bool g_fPrintedSleeping = false;

// the cycle time to use
unsigned gTxCycle;
// remaining before we reset to default
unsigned gTxCycleCount;

static Arduino_LoRaWAN::ReceivePortBufferCbFn receiveMessage;

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
        setup_bme680();
        run_bme680();
        setup_uplink();
        }

void setup_platform(void)
        {
        // if running unattended, don't wait for USB connect.
        if (! (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fUnattended)))
                {
                while (!Serial)
                        /* wait for USB attach */
                        yield();
                }

        gCatena.SafePrintf("\n");
        gCatena.SafePrintf("-------------------------------------------------------------------------------\n");
        gCatena.SafePrintf("This is the catena4460_bsec_ulp program V%s.\n", sVersion);
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

        gLoRaWAN.SetReceiveBufferBufferCb(receiveMessage);
        setTxCycleTime(CATCFG_T_CYCLE_INITIAL, CATCFG_INTERVAL_COUNT_INITIAL);

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

        gBH1750.begin();
        gBH1750.configure(BH1750_POWER_DOWN);
        }

void setup_bme680(void)
        {
        iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
        gCatena.SafePrintf("\nBSEC library version %d.%d.%d.%d\n",
                           iaqSensor.version.major,
                           iaqSensor.version.minor,
                           iaqSensor.version.major_bugfix,
                           iaqSensor.version.minor_bugfix
                           );
        checkIaqSensorStatus();

        iaqSensor.setConfig(bsec_config_iaq);
        checkIaqSensorStatus();

        loadCalibrationData();

        bsec_virtual_sensor_t sensorList[7] =
                {
                BSEC_OUTPUT_RAW_TEMPERATURE,
                BSEC_OUTPUT_RAW_PRESSURE,
                BSEC_OUTPUT_RAW_HUMIDITY,
                BSEC_OUTPUT_RAW_GAS,
                BSEC_OUTPUT_IAQ,
                BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
                BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
                };

        iaqSensor.updateSubscription(sensorList, 7, BSEC_SAMPLE_RATE_ULP);
        checkIaqSensorStatus();
        }

void run_bme680(void)
        {
        if (iaqSensor.run())
                { // If new data is available
                g_dataValid = true;
                g_temperature = iaqSensor.temperature;
                g_pressure = iaqSensor.pressure;
                g_humidity = iaqSensor.humidity;
                g_iaq = iaqSensor.iaqEstimate;
                g_iaqAccuracy = iaqSensor.iaqAccuracy;
                g_gas_resistance = iaqSensor.gasResistance;
                g_iaqValid = true;

                output = String(millis());
                output += ", " + String(iaqSensor.rawTemperature);
                output += ", " + String(iaqSensor.pressure);
                output += ", " + String(iaqSensor.rawHumidity);
                output += ", " + String(iaqSensor.gasResistance);
                output += ", " + String(iaqSensor.iaqEstimate);
                output += ", " + String(iaqSensor.iaqAccuracy);
                output += ", " + String(iaqSensor.temperature);
                output += ", " + String(iaqSensor.humidity);
                gCatena.SafePrintf("%s\n", output.c_str());

                possiblySaveCalibrationData();
                }
        else
                {
                checkIaqSensorStatus();
                }
        }
 
 void setup_uplink(void)
        {
        if (! (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest)))
                {
                if (! gLoRaWAN.IsProvisioned())
                        gCatena.SafePrintf("LoRaWAN not provisioned yet. Use the commands to set it up.\n");
                else
                        {
                        g_dataValid = false;
                        run_bme680();

                        /* trigger a join by sending the first packet */
                        startSendingUplink();
                        }
                }
        }

// Function that is looped forever
void loop(void)
        {
        gCatena.poll();

        run_bme680();
        if (gCatena.GetOperatingFlags() &
                static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest))
                {
                TxBuffer_t b;
                fillBuffer(b);
                }
        }

// Helper function definitions
void checkIaqSensorStatus(void)
        {
        if (iaqSensor.status != BSEC_OK)
                {
                if (iaqSensor.status < BSEC_OK)
                        {
                        output = "BSEC error code : " + String(iaqSensor.status);
                        gCatena.SafePrintf("%s\n", output.c_str());
                        for (;;)
                                errLeds(); /* Halt in case of failure */
                        }
                else
                        {
                        output = "BSEC warning code : " + String(iaqSensor.status);
                        gCatena.SafePrintf("%s\n", output.c_str());
                        }
                }

        if (iaqSensor.bme680Status != BME680_OK)
                {
                if (iaqSensor.bme680Status < BME680_OK)
                        {
                        output = "BME680 error code : " + String(iaqSensor.bme680Status);
                        gCatena.SafePrintf("%s\n", output.c_str());
                        for (;;)
                                errLeds(); /* Halt in case of failure */
                        }
                else
                        {
                        output = "BME680 warning code : " + String(iaqSensor.bme680Status);
                        gCatena.SafePrintf("%s\n", output.c_str());
                        }
                }
        }

void errLeds(void)
        {
        gLed.Set(LedPattern::ThreeShort);

        for (;;)
                gCatena.poll();
        }

void dumpCalibrationData(void)
        {
        for (auto here = 0, nLeft = BSEC_MAX_STATE_BLOB_SIZE;
             nLeft != 0;)
                {
                char line[160];
                size_t n;

                n = 16;
                if (n >= nLeft)
                        n = nLeft;

                McciAdkLib_FormatDumpLine(
                    line, sizeof(line), 0,
                    here,
                    bsecState + here, n);

                gCatena.SafePrintf("%s\n", line);

                here += n, nLeft -= n;
                }
        }

void loadCalibrationData(void)
        {
        cFram * const pFram = gCatena.getFram();
        bool fDataOk;

        if (pFram->getField(cFramStorage::kBme680Cal, bsecState))
                {
                gCatena.SafePrintf("Got state from FRAM:\n");
                dumpCalibrationData();

                iaqSensor.setState(bsecState);
                if (iaqSensor.status != BSEC_OK)
                        {
                        gCatena.SafePrintf(
                                "%s: BSEC error code: %d\n", __func__, iaqSensor.status
                                );
                        fDataOk = false;
                        }
                else
                        {
                        fDataOk = true;
                        }
                }
        else
                {
                fDataOk = false;
                }

        if (! fDataOk)
                {
                // initialize the saved state
                gCatena.SafePrintf("Initializing state\n");
                saveCalibrationData();
                }
        }

void possiblySaveCalibrationData(void)
        {
        bool update = false;
        if (stateUpdateCounter == 0)
                {
                /* First state update when IAQ accuracy is >= 1 (calibration complete) */
                if (iaqSensor.iaqAccuracy >= 1)
                        {
                        update = true;
                        stateUpdateCounter++;
                        }
                }
        else
                {
                /* Update every STATE_SAVE_PERIOD milliseconds */
                if (iaqSensor.iaqAccuracy >= 1 &&
                    (uint32_t)(millis() - lastCalibrationWriteMillis) >= STATE_SAVE_PERIOD)
                        {
                        update = true;
                        stateUpdateCounter++;
                        }
                }

        if (update)
                {
                saveCalibrationData();
                }
        }

void saveCalibrationData(void)
        {
        lastCalibrationWriteMillis = millis();
        iaqSensor.getState(bsecState);
        checkIaqSensorStatus();

        gCatena.SafePrintf("Writing state to FRAM\n");
        gCatena.getFram()->saveField(cFramStorage::kBme680Cal, bsecState);
        dumpCalibrationData();
        }

static uint16_t f2uflt16(
        float f
        )
        {
        if (f < 0)
                return 0;
        else if (f >= 1.0)
                return 0xFFFF;
        else
                {
                int iExp;
                float normalValue;
                normalValue = frexpf(f, &iExp);

                // f is supposed to be in [0..1), so useful exp
                // is [0..-15]
                iExp += 15;
                if (iExp < 0)
                        iExp = 0;
                if (iExp > 15)
                        return 0xFFFF;


                return (uint16_t)((iExp << 12u) + (unsigned) scalbnf(normalValue, 12));
                }
        }

void fillBuffer(TxBuffer_t &b)
        {
        b.begin();
        FlagsSensor5 flag;

        flag = FlagsSensor5(0);

        b.put(FormatSensor5); /* the flag for this record format */

        /* capture the address of the flag byte */
        uint8_t * const pFlag = b.getp();

        b.put(0x00);    /* insert a byte. Value doesn't matter, will
                        || later be set to the final value of `flag`
                        */

        // vBat is sent as 5000 * v
        float vBat = gCatena.ReadVbat();
        gCatena.SafePrintf("vBat:    %d mV\n", (int) (vBat * 1000.0f));
        b.putV(vBat);
        flag |= FlagsSensor5::FlagVbat;

        uint32_t bootCount;
        if (gCatena.getBootCount(bootCount))
                {
                b.putBootCountLsb(bootCount);
                flag |= FlagsSensor5::FlagBoot;
                }

        if (g_dataValid)
                {
                // temperature is 2 bytes from -0x80.00 to +0x7F.FF degrees C
                // pressure is 2 bytes, hPa * 10.
                // humidity is one byte, where 0 == 0/256 and 0xFF == 255/256.
                gCatena.SafePrintf(
                        "BME680:  T=%d P=%d RH=%d\n",
                        (int) (g_temperature + 0.5),
                        (int) (g_pressure + 0.5),
                        (int) (g_humidity + 0.5)
                        );
                b.putT(g_temperature);
                b.putP(g_pressure);
                b.putRH(g_humidity);

                flag |= FlagsSensor5::FlagTPH;
                }

        /* get light */
                {
                /* Get a new sensor event */
                uint16_t light;

                gBH1750.configure(BH1750_ONE_TIME_HIGH_RES_MODE);
                uint32_t const tBegin = millis();
                uint32_t const tMeas = gBH1750.getMeasurementMillis();

                while (uint32_t(millis() - tBegin) < tMeas)
                        {
                        gCatena.poll();
                        yield();
                        }

                light = gBH1750.readLightLevel();
                gCatena.SafePrintf("BH1750:  %u lux\n", light);
                b.putLux(light);
                flag |= FlagsSensor5::FlagLux;
                }

        if (g_dataValid)
                {
                // output the IAQ (in 0..500/512).
                uint16_t const encodedIAQ = f2uflt16(g_iaq / 512.0);

                gCatena.SafePrintf(
                    "BME680:   Gas-Resistance=%d IAQ=%d\n",
                    (int) (g_gas_resistance + 0.5),
                    (int) (g_iaq + 0.5)
                    );
                if (g_iaqValid)
                        {
                        b.put2u(encodedIAQ);
                        flag |= FlagsSensor5::FlagAqi;
                        }

                if (g_gas_resistance > 1.0 && g_gas_resistance < 1e15)
                        {
                        float log_gas_resistance = log10f(g_gas_resistance);

                        uint16_t const encodedLogGasR = f2uflt16(log_gas_resistance / 16.0);
                        b.put2u(encodedLogGasR);
                        flag |= FlagsSensor5::FlagLogGasR;
                        }
                if (g_iaqValid)
                        {
                        b.put(g_iaqAccuracy & 0x03);
                        flag |= FlagsSensor5::FlagAqiAccuracyMisc;
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

        gLoRaWAN.SendBuffer(b.getbase(), b.getn(), sendBufferDoneCb, nullptr, fConfirmed);
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

                        // but prevent an attempt to join.
                        gLoRaWAN.Shutdown();
                        }
                else
                        gCatena.SafePrintf("send buffer failed\n");
                }
        
        os_setTimedCallback(
                &sensorJob,
                os_getTime() + sec2osticks(CATCFG_T_SETTLE),
                pFn
                );
        }

static void txNotProvisionedCb(
        osjob_t *pSendJob
        )
        {
        gCatena.SafePrintf("not provisioned, idling\n");
        gLed.Set(LedPattern::NotProvisioned);
        }

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
        else if (Serial.dtr() || (gCatena.GetOperatingFlags() & (1 << 17)))
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
                                        run_bme680();
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
        
        /* if we can't sleep deeply, then simply schedule the sleepDoneCb */
        if (! fDeepSleep)
                {
                uint32_t interval = sec2osticks(CATCFG_GetInterval(gTxCycle));

                if (gCatena.GetOperatingFlags() & (1 << 18))
                        interval = 1;

                gLed.Set(LedPattern::Sleeping);
                os_setTimedCallback(
                        &sensorJob,
                        os_getTime() + interval,
                        sleepDoneCb
                        );
                return;
                }

        /*
        || ok... now it's time for a deep sleep. Do the sleep here, since
        || the Arduino loop won't do it. Note that nothing will get polled
        || while we sleep. We need to poll the bme680 every 3 seconds, so
        || we sleep as much as we can.
        */
        const auto tSampleSeconds = 3;
        uint32_t sleepInterval = CATCFG_GetInterval(
                        fDeepSleepTest ? CATCFG_T_CYCLE_TEST : CATCFG_T_CYCLE
                        );

        while (sleepInterval > 0)
                {
                uint32_t nThisTime;

                nThisTime = tSampleSeconds;
                if (nThisTime > sleepInterval)
                        nThisTime = sleepInterval;

                sleepInterval -= nThisTime;

                gLed.Set(LedPattern::Off);

                USBDevice.detach();

                startTime = millis();
                gRtc.SetAlarm(nThisTime);
                gRtc.SleepForAlarm(
                        gRtc.MATCH_HHMMSS,
                        gRtc.SleepMode::DeepSleep
                        );

                // add the number of ms that we were asleep to the millisecond timer.
                // we don't need extreme accuracy.
                adjust_millis_forward(nThisTime * 1000);

                gLed.Set(LedPattern::Measuring);

                /* we're not convinced that it's safe to leave USB off, so turn it on */
                USBDevice.attach();
                run_bme680();
                }

        /* and now... we're fully awake. Go to next state. */
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
                // mac downlink mesage; do nothing.
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
