/* catena4450m101_sensor.ino	Tue Mar 28 2017 19:52:20 tmm */

/*

Module:  catena4450m101_sensor.ino

Function:
	Code for the electric sensor with Catena 4450-M101.

Version:
	V0.1.1	Tue Mar 28 2017 19:52:20 tmm	Edit level 1

Copyright notice:
	This file copyright (C) 2017 by

		MCCI Corporation
		3520 Krums Corners Road
		Ithaca, NY  14850

	An unpublished work.  All rights reserved.

	This file is proprietary information, and may not be disclosed or
	copied without the prior permission of MCCI Corporation.

Author:
	Terry Moore, MCCI Corporation	March 2017

Revision history:
   0.1.0  Fri Mar 10 2017 21:42:21  tmm
	Module created.

   0.1.1  Tue Mar 28 2017 19:52:20  tmm
	Fix bug: not reading current lux value because sensor was not in
	continuous mode. Add 150uA of current draw.

*/

#include <Catena4450.h>
#include <CatenaRTC.h>
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

#include <type_traits>

/****************************************************************************\
|
|		Manifest constants & typedefs.
|
|	This is strictly for private types and constants which will not
|	be exported.
|
\****************************************************************************/

using namespace McciCatena;
using ThisCatena = Catena4450;

/* how long do we wait between measurements (in seconds) */
enum    {
        // set this to interval between measurements, in seconds
        // Actual time will be a little longer because have to
        // add measurement and broadcast time.
        CATCFG_T_CYCLE = 6 * 60,        // ten messages/hour
        CATCFG_T_WARMUP = 1,
        CATCFG_T_SETTLE = 5,
        CATCFG_T_INTERVAL = CATCFG_T_CYCLE - (CATCFG_T_WARMUP +
                                                CATCFG_T_SETTLE),
        };

/* the mask of inputs that need pullups */
enum    {
        // which ports need pullup enabled?
        CATCFG_PULLUP_BITS = 0x284c34,  // bits 2, 4, 5, 10, 11, 14, 19, 21
        };

// forwards
static void settleDoneCb(osjob_t *pSendJob);
static void warmupDoneCb(osjob_t *pSendJob);
static void txFailedDoneCb(osjob_t *pSendJob);
static void sleepDoneCb(osjob_t *pSendJob);
static Arduino_LoRaWAN::SendBufferCbFn sendBufferDoneCb;

/****************************************************************************\
|
|	Read-only data.
|
|	If program is to be ROM-able, these must all be tagged read-only
|	using the ROM storage class; they may be global.
|
\****************************************************************************/

static const char sVersion[] = "0.1.0";

/****************************************************************************\
|
|	VARIABLES:
|
|	If program is to be ROM-able, these must be initialized
|	using the BSS keyword.  (This allows for compilers that require
|	every variable to have an initializer.)  Note that only those
|	variables owned by this module should be declared here, using the BSS
|	keyword; this allows for linkers that dislike multiple declarations
|	of objects.
|
\****************************************************************************/

// globals
ThisCatena gCatena;

//
// the LoRaWAN backhaul.  Note that we use the
// ThisCatena version so it can provide hardware-specific
// information to the base class.
//
ThisCatena::LoRaWAN gLoRaWAN;

//
// the LED
//
StatusLed gLed (ThisCatena::PIN_STATUS_LED);

// the RTC instance, used for sleeping
CatenaRTC gRtc;

//   The temperature/humidity sensor
Adafruit_BME280 bme; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
BH1750 bh1750;
bool fLux;

//   The contact sensors
bool fHasPower1;
uint8_t kPinPower1P1;
uint8_t kPinPower1P2;

cTotalizer gPower1P1;
cTotalizer gPower1P2;

//  the job that's used to synchronize us with the LMIC code
static osjob_t sensorJob;
void sensorJob_cb(osjob_t *pJob);

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

        gCatena.SafePrintf("Catena 4450 sensor1 V%s\n", sVersion);

        gLed.begin();
        gCatena.registerObject(&gLed);

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
                gCatena.SafePrintf("OK\n");
                gCatena.registerObject(&gLoRaWAN);
                }

        ThisCatena::UniqueID_string_t CpuIDstring;

        gCatena.SafePrintf("CPU Unique ID: %s\n",
                gCatena.GetUniqueIDstring(&CpuIDstring)
                );

        /* find the platform */
        const ThisCatena::EUI64_buffer_t *pSysEUI = gCatena.GetSysEUI();

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


        /* initialize the lux sensor */
        if (flags & CatenaSamd21::fHasLuxRohm)
                {
                bh1750.begin();
                fLux = true;
                bh1750.configure(BH1750_CONTINUOUS_HIGH_RES_MODE_2);
                }
        else
                {
                fLux = false;
                }

        /* initialize the BME280 */
        if (!bme.begin(BME280_ADDRESS, Adafruit_BME280::OPERATING_MODE::Sleep))
                {
                gCatena.SafePrintf("No BME280 found: check wiring\n");
                fBme = false;
                }
        else
                {
                fBme = true;
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
                else
                        {
                        gCatena.SafePrintf("unknown mod number %d\n", modnumber);
                        }
                }
        else
                {
                gCatena.SafePrintf("No mods detected\n");
                }

        if (fHasPower1)
                {
                if (! gPower1P1.begin(kPinPower1P1) ||
                    ! gPower1P2.begin(kPinPower1P2))
                        {
                        fHasPower1 = false;
                        }
                }

        /* now, we kick off things by sending our first message */
        gLed.Set(LedPattern::Joining);

        /* warm up the BME280 by discarding a measurement */
        if (fBme)
                (void)bme.readTemperature();

        /* trigger a join by sending the first packet */
        startSendingUplink();
        }

// The Arduino loop routine -- in our case, we just drive the other loops.
// If we try to do too much, we can break the LMIC radio. So the work is
// done by outcalls scheduled from the LMIC os loop.
void loop()
        {
        gCatena.poll();
        }

void startSendingUplink(void)
{
  TxBuffer_t b;
  LedPattern savedLed = gLed.Set(LedPattern::Measuring);

  b.begin();
  FlagsSensor2 flag;

  flag = FlagsSensor2(0);

  b.put(FormatSensor2); /* the flag for this record format */
  uint8_t * const pFlag = b.getp();
  b.put(0x00); /* will be set to the flags */

  // vBat is sent as 5000 * v
  float vBat = gCatena.ReadVbat();
  gCatena.SafePrintf("vBat:    %d mV\n", (int) (vBat * 1000.0f));
  b.putV(vBat);
  flag |= FlagsSensor2::FlagVbat;

  uint32_t bootCount;
  if (gCatena.getBootCount(bootCount))
        {
        b.putBootCountLsb(bootCount);
        flag |= FlagsSensor2::FlagBoot;
        }

  if (fBme)
       {
       Adafruit_BME280::Measurements m = bme.readTemperaturePressureHumidity();
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

       flag |= FlagsSensor2::FlagTPH;
       }

  if (fLux)
        {
        /* Get a new sensor event */
        uint16_t light;

        light = bh1750.readLightLevel();
        gCatena.SafePrintf("BH1750:  %u lux\n", light);
        b.putLux(light);
        flag |= FlagsSensor2::FlagLux;
        }

  if (fHasPower1)
        {
        uint32_t power1in, power1out;

        power1in = gPower1P1.getcurrent();
        power1out = gPower1P2.getcurrent();

        gCatena.SafePrintf(
                "Power:   IN: %u  OUT: %u\n",
                power1in,
                power1out
                );
        b.putWH(power1in);
        b.putWH(power1out);

        flag |= FlagsSensor2::FlagWattHours;
        }

  *pFlag = uint8_t(flag);
  if (savedLed != LedPattern::Joining)
          gLed.Set(LedPattern::Sending);
  else
          gLed.Set(LedPattern::Joining);

  gLoRaWAN.SendBuffer(b.getbase(), b.getn(), sendBufferDoneCb, NULL);
}

static void
sendBufferDoneCb(
    void *pContext,
    bool fStatus
    )
    {
    osjobcb_t pFn;

    gLed.Set(LedPattern::Settling);
    if (! fStatus)
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
            os_getTime()+sec2osticks(CATCFG_T_SETTLE),
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

#include <delay.h>

static void settleDoneCb(
    osjob_t *pSendJob
    )
    {
    uint32_t startTime;

    // if connected to USB, don't sleep
    // ditto if we're monitoring pulses.
    if (Serial.dtr() || fHasPower1)
        {
        gLed.Set(LedPattern::Sleeping);
        os_setTimedCallback(
                &sensorJob,
                os_getTime() + sec2osticks(CATCFG_T_INTERVAL),
                sleepDoneCb
                );
        return;
        }

    /* ok... now it's time for a deep sleep */
    gLed.Set(LedPattern::Off);

    startTime = millis();
    gRtc.SetAlarm(CATCFG_T_INTERVAL);
    gRtc.SleepForAlarm(
        CatenaRTC::MATCH_HHMMSS,
        CatenaRTC::SleepMode::IdleCpuAhbApb
        );
    adjust_millis_forward(CATCFG_T_INTERVAL  * 1000);

    /* and now... we're awake again. trigger another measurement */
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
