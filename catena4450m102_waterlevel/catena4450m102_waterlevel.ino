/* catena4450m102_waterlevel.ino	Fri Apr 12 2019 13:25:48 dhineshkumar */

/*

Module:  catena4450m102_waterlevel.ino

Function:
	Code for the Water pressure transducer with Catena 4450-M102.

Version:
	V0.2.1	Fri Apr 12 2019 13:25:48 dhineshkumar	Edit level 5

Copyright notice:
	This file copyright (C) 2017-19 by

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

   0.1.8  Sat Nov 11 2017 18:58:23  tmm
        Rebuild with latest libraries to fix T < 0 deg C encoding problem.
		
   0.2.0 Sat Feb 03 2018 16:11:23 vel
	Adding code to interface water pressure transducer.

   0.2.1  Fri Apr 12 2019 13:25:48  dhineshkumar
        Added sensor setup and txCycle to the application.

*/

#include <Catena.h>

#ifdef ARDUINO_ARCH_SAMD
# include <CatenaRTC.h>
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

#ifdef ARDUINO_ARCH_SAMD
# include <delay.h>
#endif

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
using ThisCatena = Catena;

/* how long do we wait between transmissions? (in seconds) */
enum	{
	// set this to interval between transmissions, in seconds
	// Actual time will be a little longer because have to
	// add measurement and broadcast time, but we attempt
	// to compensate for the gross effects below.
	CATCFG_T_CYCLE =  30,        // every 6 minutes
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

// forwards
static void settleDoneCb(osjob_t *pSendJob);
static void warmupDoneCb(osjob_t *pSendJob);
static void txFailedDoneCb(osjob_t *pSendJob);
static void sleepDoneCb(osjob_t *pSendJob);
static Arduino_LoRaWAN::SendBufferCbFn sendBufferDoneCb;
void fillBuffer(TxBuffer_t &b);
void startSendingUplink(void);
void sensorJob_cb(osjob_t *pJob);
void setTxCycleTime(unsigned txCycle, unsigned txCount);

//Auro Water Pressure sensor value calcultion formula macros
#define REFERENCE_VALUE (1677721)
#define CROSS_VALUE (13421772)
#define RANGE (1000)

/****************************************************************************\
|
|	Read-only data.
|
|	If program is to be ROM-able, these must all be tagged read-only
|	using the ROM storage class; they may be global.
|
\****************************************************************************/

static const char sVersion[] = "0.2.0";

//
// set this to the branch you're using, if this is a branch.
static const char sBranch[] = "";
// keep by itself, more or less, for easy git rebasing.
//
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

#ifdef ARDUINO_ARCH_SAMD
// the RTC instance, used for sleeping
CatenaRTC gRtc;
#endif /* ARDUINO_ARCH_SAMD*/


//Auro RC610 Water Pressure Sensor Activated
bool AuroPressureSensor = true;

//   The temperature/humidity sensor
Adafruit_BME280 gBME280; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
BH1750 gBH1750;
bool fLux;

//   The contact sensors
bool fHasPower1;
uint8_t kPinPower1P1;
uint8_t kPinPower1P2;

cTotalizer gPower1P1;
cTotalizer gPower1P2;

//  the job that's used to synchronize us with the LMIC code
static osjob_t sensorJob;

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
	setup_sensors();

	/* for 4551, we need wider tolerances, it seems */
#if defined(ARDUINO_ARCH_STM32)
	LMIC_setClockError(10 * 65536 / 100);
#endif

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
	gCatena.SafePrintf("This is the catena4450m102_pond program V%s%s.\n", sVersion, sBranch);
		{
		char sRegion[16];
		gCatena.SafePrintf("Target network: %s / %s\n",
				gLoRaWAN.GetNetworkName(),
				gLoRaWAN.GetRegionString(sRegion, sizeof(sRegion))
				);
		}
	gCatena.SafePrintf("Current board: %s\n", gCatena.CatenaName());
	gCatena.SafePrintf("Enter 'help' for a list of commands.\n");
	gCatena.SafePrintf("(remember to select 'Line Ending: Newline' at the bottom of the monitor window.)\n");
	gCatena.SafePrintf("--------------------------------------------------------------------------------\n");
	gCatena.SafePrintf("\n");

	// set up the LED
	gLed.begin();
	gCatena.registerObject(&gLed);
	gLed.Set(LedPattern::FastFlash);

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
		else
			{
			gCatena.SafePrintf("unknown mod number %d\n", modnumber);
			}
		}
	else
		{
		gCatena.SafePrintf("No mods detected\n");
		}

	/* now, we kick off things by sending our first message */
	gLed.Set(LedPattern::Joining);

	/* warm up the BME280 by discarding a measurement */
	if (fBme)
		(void)gBME280.readTemperature();
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
	}

// The Arduino loop routine -- in our case, we just drive the other loops.
// If we try to do too much, we can break the LMIC radio. So the work is
// done by outcalls scheduled from the LMIC os loop.
void loop()
	{
	gCatena.poll();
	if (gCatena.GetOperatingFlags() &
		static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest))
		{
		TxBuffer_t b;
		fillBuffer(b);
		}
	}

void fillBuffer(TxBuffer_t &b)
	{
	b.begin();
	FlagsSensor4 flag;

	flag = FlagsSensor4(0);

	b.put(FormatSensor4); /* the flag for this record format */
	uint8_t * const pFlag = b.getp();
	b.put(0x00); /* will be set to the flags */

	// vBat is sent as 5000 * v
	float vBat = gCatena.ReadVbat();
	gCatena.SafePrintf("vBat:    %d mV\n", (int) (vBat * 1000.0f));
	b.putV(vBat);
	flag |= FlagsSensor4::FlagVbat;

	uint32_t bootCount;
	if (gCatena.getBootCount(bootCount))
		{
		b.putBootCountLsb(bootCount);
		flag |= FlagsSensor4::FlagBoot;
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

		flag |= FlagsSensor4::FlagTPH;
		}

	if (fLux)
		{
		/* Get a new sensor event */
		uint16_t light;

		light = gBH1750.readLightLevel();
		gCatena.SafePrintf("BH1750:  %u lux\n", light);
		b.putLux(light);
		flag |= FlagsSensor4::FlagLux;
		}
		
/*
|| Measure and transmit the RC601 water pressure transducer values.
|| No seperate library cerated. Used the I2C wire library controls.
|| Calculation formula of current pressure value of transmitter:
|| current pressure value = （current read value - reference value）/（cross value）*（transmitter range）
|| The reference value is fixed to 1677721; The cross value is fixed to 13421772;
|| The calculation of the output pressure value of 1000KPa transmitter is
|| as follows:
|| If the current read data is 3100429,
|| Calculation：（3100429-1677721）/（13421772）*（1000KPa）= 106KPa
*/
	if (AuroPressureSensor)
		{
		unsigned char pBuffer[10]={0};
		uint8_t Index, nBytes;
		uint32_t SenAD;
		float SenPDat;
		
		/* Initialize the I2C MUX and Choose the second channel*/
		Wire.beginTransmission(0x70);
		Wire.write(0x05); 
		Wire.endTransmission();
		delay(1000);
		
		/* Sending acquisition instruction to Sensor*/
		Wire.beginTransmission(0); 
		Wire.write(0xaa);
		Wire.write(0x00);
		Wire.write(0x00);
		Wire.endTransmission();
		delay(100);
		
		/* 
		Reading the output data frame contains: status, pressure and temperature
		data; total 8 bytes; The first byte of the output frame is a status
		byte; The 2~4 byte is 24 bit unsigned pressure data, high byte in front and
		low byte in back; The 5~7 byte is 24 bit unsigned temperature data, high byte
		in front, low byte in back 
		*/
		nBytes = 7;
		Wire.requestFrom(0, nBytes);

		if (nBytes <= Wire.available())
			{ 
			/* Reading the nBytes of Data and storing it internal buffer*/
			Index = 0;
			while (Index < nBytes)
				{
				pBuffer[Index++] = Wire.read();
				}
			}
		
		SenAD = (uint32_t) ((pBuffer[1] << 16) + (pBuffer[2] << 8) + pBuffer[3]);
		gCatena.SafePrintf("SenAD: %u\n", SenAD);
		
		if (SenAD >= REFERENCE_VALUE) /* Check for negative value, If yes then print 0*/
			{
			SenPDat = (float)(SenAD - REFERENCE_VALUE) / CROSS_VALUE * RANGE;
			gCatena.SafePrintf("SenPDat: %d Kpa\n", (int)SenPDat);
			
			b.putP(SenPDat * 1000);
			flag |= FlagsSensor4::FlagWaterPressure;
			}
		else
			{
			gCatena.SafePrintf("SenAD: 0.00 Kpa\n");
			
			b.put2uf(0);
			flag |= FlagsSensor4::FlagWaterPressure; 
			}
		delay(1000);
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

static void sendBufferDoneCb(
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

static void txFailedDoneCb(
	osjob_t *pSendJob
	)
	{
	gCatena.SafePrintf("not provisioned, idling\n");
	gLoRaWAN.Shutdown();
	gLed.Set(LedPattern::NotProvisioned);
	}


//
// the following API is added to delay.c, .h in the BSP. It adjust millis()
// forward after a deep sleep.
//
// extern "C" { void adjust_millis_forward(unsigned); };
//
// If you don't have it, check the following commit at github:
// https://github.com/mcci-catena/ArduinoCore-samd/commit/78d8440dbcd29bf5ac659fd65514268c1334f683
//

static void settleDoneCb(
	osjob_t *pSendJob
	)
	{
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

	// update the sleep parameters
	if (gTxCycleCount > 1)
		--gTxCycleCount;
	else
		{
		if (gTxCycleCount > 0)
			gCatena.SafePrintf("resetting tx cycle to default: %u\n", CATCFG_T_CYCLE);

		gTxCycleCount = 0;
		gTxCycle = CATCFG_T_CYCLE;
		}

	// if not permitted to sleep, just go on as normal.
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
	|| ok... now it's time for a deep sleep.  Do the sleep here, since
	|| the Arduino loop won't do it. Note that nothing will get polled
	|| while we sleep. We can't poll if we're polling power.
	*/
	gLed.Set(LedPattern::Off);

#ifdef ARDUINO_ARCH_SAMD
	USBDevice.detach();
#else
	Serial.end();
#endif
	Wire.end();

	uint32_t const sleepInterval = CATCFG_GetInterval(
			fDeepSleepTest ? CATCFG_T_CYCLE_TEST : gTxCycle
			);

	gCatena.Sleep(sleepInterval);

	Wire.begin();

#ifdef ARDUINO_ARCH_SAMD
	USBDevice.attach();
#else
	Serial.begin(115200);
#endif
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

static void receiveMessage(
	void *pContext,
	uint8_t port,
	const uint8_t *pMessage,
	size_t nMessage
	)
	{
	unsigned txCycle;
	unsigned txCount;

	if (! (port == 1 && 2 <= nMessage && nMessage <= 3))
		{
		gCatena.SafePrintf("invalid message port(%02x)/length(%zx)\n",
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
