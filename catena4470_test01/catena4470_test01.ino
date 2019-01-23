/*

Module:  catena4470_test01.ino

Function:
	Code for the modbus meter/devices with Catena 4470.

Copyright notice:
	See LICENSE file accompanying this project

Author:
	Dhinesh Kumar Pitchai, MCCI Corporation	January 2019

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
#include <Catena_ModbusRtu.h>
#include <Catena_Flash_at25sf081.h>

#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <wiring_private.h>
#include <Adafruit_BME280.h>
#include <Arduino_LoRaWAN.h>
#include <BH1750.h>
#include <lmic.h>
#include <hal/hal.h>
#include <mcciadk_baselib.h>

#include <cmath>
#include <type_traits>

/****************************************************************************\
|
|	Manifest constants & typedefs.
|
\****************************************************************************/

using namespace McciCatena;

/* how long do we wait between transmissions? (in seconds) */
enum	{
	// set this to interval between transmissions, in seconds
	// Actual time will be a little longer because have to
	// add measurement and broadcast time, but we attempt
	// to compensate for the gross effects below.
	CATCFG_T_CYCLE = 6 * 60,        // every 6 minutes
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

// function for scaling power
static uint16_t
dNdT_getFrac(
	uint32_t deltaC,
	uint32_t delta_ms
	);

/****************************************************************************\
|
|	Read-only data.
|
\****************************************************************************/

static const char sVersion[] = "0.1.0";

/****************************************************************************\
|
|	Variables.
|
\****************************************************************************/

// the Catena instance
Catena gCatena;

#define EXCEPTION_CODE		5

// data array for modbus network sharing
uint16_t au16data[16];
uint8_t u8state;
uint8_t u8addr;
uint8_t u8fct;
uint8_t u16RegAdd;
uint8_t u16CoilsNo;
uint8_t flagModBus = 0;

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for host, = 1..247 for device
 *         In this case, we're the host.
 *  u8txenpin : 0 for RS-232 and USB-FTDI
 *               or any pin number > 1 for RS-485
 *
 *  We also need a serial port object, based on a UART;
 *  we default to serial port 1.  Since the type of Serial1
 *  varies from platform to platform, we use decltype() to
 *  drive the template based on the type of Serial1,
 *  eliminating a complex and never-exhaustive series of
 *  #ifs.
 */
cCatenaModbusRtu host(0, A4); // this is host and RS-232 or USB-FTDI
ModbusSerial<decltype(Serial1)> mySerial(&Serial1);

#define kPowerOn	A3

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
CatenaStm32L0Rtc gRtc;
#endif

// The BME280 instance, for the temperature/humidity sensor
Adafruit_BME280 gBME280; // The default initalizer creates an I2C connection
bool fBme;		// true if BME280 is in use

//   The LUX sensor
BH1750 gBH1750;
bool fLux;		// true if BH1750 is in use

//   The contact sensors
bool fHasPower1;
uint8_t kPinPower1P1;
uint8_t kPinPower1P2;

cTotalizer gPower1P1;
cTotalizer gPower1P2;

modbus_t telegram;
bool fModBusSlave;
unsigned long u32wait;

//   The flash
cFlash_AT25SF081 gFlash;
bool fFlash;

// SCK is D12, which is SERCOM1.3
// MOSI is D11, which is SERCOM1.0
// MISO is D10, which is SERCOM1.2
// Knowing that, you might be able to make sens of the following:
SPIClass gSPI2(&sercom1, 10, 12, 11, SPI_PAD_0_SCK_3, SERCOM_RX_PAD_2);

//  the LMIC job that's used to synchronize us with the LMIC code
static osjob_t sensorJob;

// debug flag for throttling sleep prints
bool g_fPrintedSleeping = false;

// the cycle time to use
unsigned gTxCycle;
// remaining before we reset to default
unsigned gTxCycleCount;

static Arduino_LoRaWAN::ReceivePortBufferCbFn receiveMessage;

static inline void powerOn(void)
	{
	pinMode(kPowerOn, OUTPUT);
	digitalWrite(kPowerOn, HIGH);
	}

static inline void powerOff(void)
	{
	pinMode(kPowerOn, INPUT);
	digitalWrite(kPowerOn, LOW);
	}

/****************************************************************************\
|
|	Code.
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
	powerOn();
	setup_platform();	// setup platform
	setup_flash();		// set up flash
	setup_sensors();	// set up in-built sensors
	setup_modbus();		// set up Modbus Master

	/* for STM32 devices, we need wider tolerances, it seems */
#if defined(ARDUINO_ARCH_STM32)
	LMIC_setClockError(10 * 65536 / 100);
#endif

	/* trigger a join by sending the first packet */
	if (!(gCatena.GetOperatingFlags() &
		static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest)))
		{
		if (!gLoRaWAN.IsProvisioned())
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
	gCatena.SafePrintf("This is the catena4470_test01 program V%s.\n", sVersion);
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
		gCatena.SafePrintf("Catena 4470-M%u\n", modnumber);
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
	}

/*
Name:	setup_flash()

Function:
	Setup flash for low power mode. (Not app specific.)

Definition:
	void setup_flash(
		void
		);

Description:
	This function only setup the flash in low power mode.

Returns:
	No explicit result.

*/

void setup_flash(void)
	{
	gSPI2.begin();

	// these *must* be after gSPI2.begin(), because the SPI
	// library resets the pins to defaults as part of the begin()
	// method.
	pinPeripheral(10, PIO_SERCOM);
	pinPeripheral(12, PIO_SERCOM);
	pinPeripheral(11, PIO_SERCOM);
	
	/* initialize the FLASH */
	gCatena.SafePrintf("Init FLASH\n");
	
	if (gFlash.begin(&gSPI2, 5))
		{
		uint8_t ManufacturerId;
		uint16_t DeviceId;

		gFlash.readId(&ManufacturerId, &DeviceId);
		gCatena.SafePrintf(
			"FLASH found, ManufacturerId=%02x, DeviceId=%04x\n",
			ManufacturerId, DeviceId
			);
		fFlash = true;
		gFlash.powerDown();
		}
	else
		{
		fFlash = false;
		gSPI2.end();
	gCatena.SafePrintf("No FLASH found: check wiring\n");
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

	/* initialize the pulse counters */
	if (fHasPower1)
		{
		if (! gPower1P1.begin(kPinPower1P1) ||
		    ! gPower1P2.begin(kPinPower1P2))
			{
			fHasPower1 = false;
			}
		}
	}

/*

Name:	setup_modbus()

Function:
	Set up the modbus we intend to use (app specific).

Definition:
	void setup_modbus(
		void
		);

Description:
	This function only exists to make clear what has to be done for
	the actual application. This is the code that cannot be part of
	the generic gCatena.begin() function.

Returns:
	No explicit result.

*/

void setup_modbus(void)
	{
	/* Modbus slave parameters */
	u8addr = 65;      // current target device: 65
	u8fct = 03;      // Function code
	u16RegAdd = 00;  // start address in device
	u16CoilsNo = 13; // number of elements (coils or registers) to read
	host.begin(&mySerial, 9600);	// baud-rate at 9600
	host.setTimeOut(1000);	// if there is no answer in 10000 ms, roll over
	host.setTxEnableDelay(100);	// wait 1000ms before each tx
	
	gCatena.registerObject(&host);	// enroll the host object in the poll list.

	// set up the fsm
	u32wait = millis() + 1000;	// when to exit state 0
	
	gCatena.SafePrintf("In this example Catena has been configured as Modbus master with:\n");
	gCatena.SafePrintf("- Slave ID: %d\n", u8addr);
	gCatena.SafePrintf("- Function Code: %d\n", u8fct);
	gCatena.SafePrintf("- Start Address: %d\n", u16RegAdd);
	gCatena.SafePrintf("- Number of Registers: %d\n", u16CoilsNo);
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

	void fillBuffer(TxBuffer_t &b)
	{
	uint32_t tLuxBegin;

	/* start a Lux measurement */
	if (fLux)
		{
		gBH1750.configure(BH1750_ONE_TIME_HIGH_RES_MODE);
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

	uint8_t nIndex;
	ERR_LIST lastError;
	uint8_t errorStatus;
	errorStatus = 1;
	u8state = 0;

	while (u8state != EXCEPTION_CODE)
		{
		switch( u8state ) {
		case 0:
			if (long(millis() - u32wait) > 0) u8state++; // wait state
			break;
		case 1:
			telegram.u8id = u8addr; // device address
			telegram.u8fct = u8fct; // function code (this one is registers read)
			telegram.u16RegAdd = u16RegAdd; // start address in device
			telegram.u16CoilsNo = u16CoilsNo; // number of elements (coils or registers) to read
			telegram.au16reg = au16data; // pointer to a memory array in the Arduino

			host.setLastError(ERR_SUCCESS);
			host.query( telegram ); // send query (only once)
			u8state++;
			break;
		case 2:
			host.poll();
			if (host.getState() == COM_IDLE)
				{
				flagModBus = 1;
				u8state = EXCEPTION_CODE;	//Exception to break out of current WHILE loop.
				
				ERR_LIST lastError = host.getLastError();

				if (host.getLastError() != ERR_SUCCESS) {
					Serial.print("Error: ");
					Serial.println(int(lastError));
					flagModBus = 0;
					break;
					}
				else {
					Serial.print("Registers: ");
					for (nIndex = 0; nIndex < 13; ++nIndex)
						{
						Serial.print(" ");
						Serial.print(au16data[nIndex], 16);
						b.put2((uint32_t)au16data[nIndex]);
						}
					}
				Serial.println("");

				flag |= FlagsSensor3::FlagWater;

				u32wait = millis() + 100;
				}
			else
				{
				if(errorStatus == 1)
					{
					gCatena.SafePrintf("Please wait while searching for MODBUS SLAVE\n");
					errorStatus = 0;
					} 
				}
			break;
			}
		}
		
	if(flagModBus == 0)
		{
		if (lastError == -4)
			{
			gCatena.SafePrintf("ERROR: BAD CRC\n");
			}
		if (lastError == -6)
			{
			gCatena.SafePrintf("ERROR: NO REPLY (OR) MODBUS SLAVE UNAVAILABLE\n");
			}
		if (lastError == -7)
			{
			gCatena.SafePrintf("ERROR: RUNT PACKET\n");
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

	// if connected to USB, don't sleep
	// ditto if we're monitoring pulses.
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
	USBDevice.detach();
	powerOff();

	startTime = millis();
	uint32_t const sleepInterval = CATCFG_GetInterval(
			fDeepSleepTest ? CATCFG_T_CYCLE_TEST : gTxCycle
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
	powerOn();

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
