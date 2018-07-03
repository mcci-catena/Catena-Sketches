/* catena4551_test01.ino	Tue Jul 03 2018 20:18:23 vel */

/*

Module:  catena4551_test01.ino

Function:
	Test program for Catena 4551 and variants.

Version:
	V0.8.0	Tue Jul 03 2018 20:18:23 vel	Edit level 4

Copyright notice:
	This file copyright (C) 2017 by

		MCCI Corporation
		3520 Krums Corners Road
		Ithaca, NY  14850

	An unpublished work.  All rights reserved.

	This file is proprietary information, and may not be disclosed or
	copied without the prior permission of MCCI Corporation

Author:
	ChaeHee Won, MCCI Corporation	October 2017

Revision history:
   0.1.0  Thu Oct 19 2017 13:10:58  chwon
	Module created.

   0.6.0  Wed Dec 06 2017 15:35:20  chwon
	Improve power management.

   0.7.0  Wed Jan 03 2018 11:09:14  chwon
	Add USB power check.
	
   0.8.0 Wed Tue Jul 03 2018 20:18:23 vel
    Fixed Lux sensor initalization issue

*/

#include "ThisCatena.h"

#include <Catena_Led.h>
#include <Catena_TxBuffer.h>
#include <Catena_CommandStream.h>
#include <Catena_Totalizer.h>
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

/****************************************************************************\
|
|		Manifest constants & typedefs.
|
|	This is strictly for private types and constants which will not
|	be exported.
|
\****************************************************************************/

using namespace McciCatena;

/* how long do we wait between measurements (in seconds) */
enum    {
	// set this to interval between measurements, in seconds
	// Actual time will be a little longer because have to
	// add measurement and broadcast time.
	//CATCFG_T_CYCLE = 6 * 60,        // ten messages/hour
	CATCFG_T_CYCLE = 30,
	CATCFG_T_WARMUP = 1,
	CATCFG_T_SETTLE = 5,
	CATCFG_T_INTERVAL = CATCFG_T_CYCLE - (CATCFG_T_WARMUP +
						CATCFG_T_SETTLE),
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
CatenaStm32L0Rtc gRtc;

//   The temperature/humidity sensor
Adafruit_BME280 gBme; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
Catena_Si1133 gSi1133;
bool fLux;

//   The contact sensors
bool fHasPower1;
uint8_t kPinPower1P1;
uint8_t kPinPower1P2;

cTotalizer gPower1P1;
cTotalizer gPower1P2;

SPIClass gSPI2(
		ThisCatena::PIN_SPI2_MOSI,
		ThisCatena::PIN_SPI2_MISO,
		ThisCatena::PIN_SPI2_SCK
		);

//   The flash
Catena_Mx25v8035f gFlash;
bool fFlash;

//  USB power
bool fUsbPower;

//  the job that's used to synchronize us with the LMIC code
static osjob_t sensorJob;
void sensorJob_cb(osjob_t *pJob);

// function for scaling power
static uint16_t
dNdT_getFrac(
	uint32_t deltaC,
	uint32_t delta_ms
	);

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

	gCatena.SafePrintf("Catena 4551 test01 V%s\n", sVersion);

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

	gCatena.SafePrintf(
		"CPU Unique ID: %s\n",
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
			gCatena.SafePrintf(
				"%s%02x", i == 0 ? "" : "-",
				pSysEUI->b[i]
				);
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
	if (flags & CatenaStm32::fHasLuxSi1133)
		{
		if (gSi1133.begin())
			{
			fLux = true;
			gSi1133.configure(0, CATENA_SI1133_MODE_SmallIR);
			gSi1133.configure(1, CATENA_SI1133_MODE_White);
			gSi1133.configure(2, CATENA_SI1133_MODE_UV);
			gSi1133.start();
			}
		else
			{
			fLux = false;
			gCatena.SafePrintf("No Si1133 found: check wiring\n");
			}
		}
	else
		{
		fLux = false;
		}

	/* initialize the BME280 */
	if (flags & CatenaStm32::fHasBme280)
		{
		if (gBme.begin())
			{
			fBme = true;
			}
		else
			{
			fBme = false;
			gCatena.SafePrintf("No BME280 found: check wiring\n");
			}
		}
	else
		{
		fBme = false;
		}

	/* initialize the FLASH */
	if (flags & CatenaStm32::fHasFlash)
		{
		if (gFlash.begin(&gSPI2, ThisCatena::PIN_SPI2_FLASH_SS))
			{
			fFlash = true;
			gFlash.powerDown();
			}
		else
			{
			fFlash = false;
			gCatena.SafePrintf("No FLASH found: check wiring\n");
			}
		}
	else
		{
		fFlash = false;
		}

	/* is it modded? */
	uint32_t modnumber = gCatena.PlatformFlags_GetModNumber(flags);

	fHasPower1 = false;

	if (modnumber != 0)
		{
		gCatena.SafePrintf("Catena 4551-M%u\n", modnumber);
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
	LMIC_setClockError(10*65536/100);

	// unit testing for the scaling functions
	//gCatena.SafePrintf(
	//        "dNdT_getFrac tests: "
	//        "0/0: %04x 90/6m: %04x 89/6:00.1: %04x 1439/6m: %04x\n",
	//        dNdT_getFrac(0, 0),
	//        dNdT_getFrac(90, 6 * 60 * 1000),
	//        dNdT_getFrac(89, 6 * 60 * 1000 + 100),
	//        dNdT_getFrac(1439, 6 * 60 * 1000)
	//        );
	//gCatena.SafePrintf(
	//        "dNdT_getFrac tests: "
	//        "1/6m: %04x 20/6m: %04x 1/60:00.1: %04x 1440/5:59.99: %04x\n",
	//        dNdT_getFrac(1, 6 * 60 * 1000),
	//        dNdT_getFrac(20, 6 * 60 * 1000),
	//        dNdT_getFrac(1, 60 * 60 * 1000 + 100),
	//        dNdT_getFrac(1440, 6 * 60 * 1000 - 10)
	//        );

	/* warm up the BME280 by discarding a measurement */
	if (fBme)
		(void) gBme.readTemperature();
	if (fLux)
		{
		delay(510);	/* default measure interval is 500ms */
		}

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

static uint16_t dNdT_getFrac(
	uint32_t deltaC,
	uint32_t delta_ms
	)
	{
	if (delta_ms == 0 || deltaC == 0)
		return 0;

	// this is a value in [0,1)
	float dNdTperHour = float(deltaC * 250) / float(delta_ms);

	if (dNdTperHour <= 0)
		return 0;
	else if (dNdTperHour >= 1)
		return 0xFFFF;
	else
		{
		int iExp;
		float normalValue;
		normalValue = frexpf(dNdTperHour, &iExp);

		// dNdTperHour is supposed to be in [0..1), so useful exp
		// is [0..-15]
		iExp += 15;
		if (iExp < 0)
			iExp = 0;
		if (iExp > 15)
			return 0xFFFF;


		return (uint16_t)((iExp << 12u) + (unsigned) scalbnf(normalValue, 12));
		}
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

	// vBus is sent as 5000 * v
	float vBus = gCatena.ReadVbus();
	gCatena.SafePrintf("vBus:    %d mV\n", (int) (vBus * 1000.0f));
	fUsbPower = (vBus > 2.0) ? true : false;

	uint32_t bootCount;
	if (gCatena.getBootCount(bootCount))
		{
		b.putBootCountLsb(bootCount);
		flag |= FlagsSensor2::FlagBoot;
		}

	if (fBme)
		{
		Adafruit_BME280::Measurements m = gBme.readTemperaturePressureHumidity();
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
		uint16_t data[3];

		gSi1133.readMultiChannelData(data, 3);
		gCatena.SafePrintf(
			"Si1133:  %u lux, %u White, %u UV\n",
			data[0],
			data[1],
			data[2]
			);
		b.putLux(data[0]);
		flag |= FlagsSensor2::FlagLux;
		}

	if (fHasPower1)
		{
		uint32_t power1in, power1out;
		uint32_t power1in_dc, power1in_dt;
		uint32_t power1out_dc, power1out_dt;

		power1in = gPower1P1.getcurrent();
		gPower1P1.getDeltaCountAndTime(power1in_dc, power1in_dt);
		gPower1P1.setReference();

		power1out = gPower1P2.getcurrent();
		gPower1P2.getDeltaCountAndTime(power1out_dc, power1out_dt);
		gPower1P2.setReference();

		gCatena.SafePrintf(
			"Power:   IN: %u OUT: %u\n",
			power1in,
			power1out
			);
		b.putWH(power1in);
		b.putWH(power1out);
		flag |= FlagsSensor2::FlagWattHours;

		// we know that we get at most 4 pulses per second, no matter
		// the scaling. Therefore, if we convert to pulses/hour, we'll
		// have a value that is no more than 3600 * 4, or 14,400.
		// This fits in 14 bits. At low pulse rates, there's more
		// info in the denominator than in the numerator. So FP is really
		// called for. We use a simple floating point format, since this
		// is unsigned: 4 bits of exponent, 12 bits of fraction, and we
		// don't bother to omit the MSB of the (normalized fraction);
		// and we multiply by 2^(exp+1) (so 0x0800 is 2 * 0.5 == 1,
		// 0xF8000 is 2^16 * 1 = 65536).
		uint16_t fracPower1In = dNdT_getFrac(power1in_dc, power1in_dt);
		uint16_t fracPower1Out = dNdT_getFrac(power1out_dc, power1out_dt);
		b.putPulseFraction(
			fracPower1In
			);
		b.putPulseFraction(
			fracPower1Out
			);

		gCatena.SafePrintf(
			"Power:   IN: %u/%u (%04x) OUT: %u/%u (%04x)\n",
			power1in_dc, power1in_dt,
			fracPower1In,
			power1out_dc, power1out_dt,
			fracPower1Out
			);

		flag |= FlagsSensor2::FlagPulsesPerHour;
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


static void settleDoneCb(
	osjob_t *pSendJob
	)
	{
	uint32_t startTime;

	// if connected to USB, don't sleep
	// ditto if we're monitoring pulses.
	if (fUsbPower || fHasPower1)
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
	gSi1133.stop();

	startTime = millis();
	gRtc.SetAlarm(CATCFG_T_INTERVAL);
	gRtc.SleepForAlarm(gRtc.SleepMode::Standby);

	// add the number of ms that we were asleep to the millisecond timer.
	// we don't need extreme accuracy.
	gRtc.AdjustMillisForward(CATCFG_T_INTERVAL * 1000);

	/* and now... we're awake again. trigger another measurement */
	gSi1133.start();
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
