/* catena461x_hwtest.ino	Thu Dec 06 2018 09:58:51 chwon */

/*

Module:  catena461x_hwtest.ino

Function:
	 Hardware test app for Catena-461x

Version:
	V0.12.0	Thu Dec 06 2018 09:58:51 chwon	Edit level 1

Copyright notice:
	This file copyright (C) 2018 by

		MCCI Corporation
		3520 Krums Corners Road
		Ithaca, NY  14850

	An unpublished work.  All rights reserved.

	This file is proprietary information, and may not be disclosed or
	copied without the prior permission of MCCI Corporation

Author:
	ChaeHee Won, MCCI Corporation	December 2018

Revision history:
   0.12.0  Thu Dec 06 2018 09:58:51  chwon
	Module created.

*/

#include <Catena.h>
#include <Catena_Led.h>
#include <Catena_TxBuffer.h>
#include <Catena_CommandStream.h>
#include <Catena_Mx25v8035f.h>
#include <Catena_Guids.h>

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Arduino_LoRaWAN.h>
#include <Catena_Si1133.h>
#include <lmic.h>
#include <hal/hal.h>
#include <mcciadk_baselib.h>

#include <cmath>
#include <type_traits>

#include <ArduinoUnit.h>

using namespace McciCatena;

/****************************************************************************\
|
|		Manifest constants & typedefs.
|
|	This is strictly for private types and constants which will not
|	be exported.
|
\****************************************************************************/

bool flash_init(void);
void setup_flash(void);
void setup_platform(void);
void adc_testing(void);


/****************************************************************************\
|
|	Read-only data.
|
|	If program is to be ROM-able, these must all be tagged read-only
|	using the ROM storage class; they may be global.
|
\****************************************************************************/


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

//   The LUX sensor
Catena_Si1133 gSi1133;

SPIClass gSPI2(
		Catena::PIN_SPI2_MOSI,
		Catena::PIN_SPI2_MISO,
		Catena::PIN_SPI2_SCK
		);

//   The flash
Catena_Mx25v8035f gFlash;

bool fHardwareTestDone;

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

	//** set up for flash work **
	setup_flash();

	fHardwareTestDone = false;
	}

void loop(void)
	{
	gCatena.poll();
	Test::run();
	if (fHardwareTestDone)
		adc_testing();
	}

void setup_platform(void)
	{
	while (! Serial)
		/* wait for USB attach */
		yield();

	Serial.print(
		"\n"
		"-------------------------------------------------------------------------------\n"
		);

	gCatena.SafePrintf(
		"This is the %s hardware test program. SYSCLK %u MHz\n",
		gCatena.CatenaName(),
		CATENA_CFG_SYSCLK
		);

	Serial.print(
		"Enter 'help' for a list of commands.\n"
		"(remember to select 'Line Ending: Newline' at the bottom of the monitor window.)\n"
		"--------------------------------------------------------------------------------\n"
		"\n"
		);

	gLed.begin();
	gCatena.registerObject(&gLed);
	gLed.Set(LedPattern::FastFlash);

	Catena::UniqueID_string_t CpuIDstring;

	gCatena.SafePrintf(
		"CPU Unique ID: %s\n",
		gCatena.GetUniqueIDstring(&CpuIDstring)
		);
	}

//-----------------------------------------------------
// platform tests
//-----------------------------------------------------

test(2_platform_05_check_platform_guid)
	{
	const CATENA_PLATFORM * const pPlatform = gCatena.GetPlatform();

	assertTrue(pPlatform != nullptr, "gCatena.GetPlatform() failed -- this is normal on first boot");

	uint32_t flags = gCatena.GetPlatformFlags();
	uint32_t modNumber = gCatena.PlatformFlags_GetModNumber(flags);

	if (modNumber != 0)
		gCatena.SafePrintf("%s-M%u\n", gCatena.CatenaName(), modNumber);
	else
		gCatena.SafePrintf("%s-BASE\n", gCatena.CatenaName());
	switch (modNumber)
		{
	case 101:
		{
		const MCCIADK_GUID_WIRE m101Guid =
#if defined(ARDUINO_MCCI_CATENA_4610)
			GUID_HW_CATENA_4610_M101(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4611)
			GUID_HW_CATENA_4611_M101(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4612)
			GUID_HW_CATENA_4612_M101(WIRE);
#else
# error not Catena 461x supported
#endif
		assertEqual(
			memcmp(&m101Guid, &pPlatform->Guid, sizeof(m101Guid)), 0,
			"platform guid mismatch"
			);
		}
		break;

	case 102:
		{
		const MCCIADK_GUID_WIRE m102Guid =
#if defined(ARDUINO_MCCI_CATENA_4610)
			GUID_HW_CATENA_4610_M102(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4611)
			GUID_HW_CATENA_4611_M102(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4612)
			GUID_HW_CATENA_4612_M102(WIRE);
#else
# error not Catena 461x supported
#endif
		assertEqual(
			memcmp(&m102Guid, &pPlatform->Guid, sizeof(m102Guid)), 0,
			"platform guid mismatch"
			);
		}
		break;

	case 103:
		{
		const MCCIADK_GUID_WIRE m103Guid =
#if defined(ARDUINO_MCCI_CATENA_4610)
			GUID_HW_CATENA_4610_M103(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4611)
			GUID_HW_CATENA_4611_M103(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4612)
			GUID_HW_CATENA_4612_M103(WIRE);
#else
# error not Catena 461x supported
#endif
		assertEqual(
			memcmp(&m103Guid, &pPlatform->Guid, sizeof(m103Guid)), 0,
			"platform guid mismatch"
			);
		}
		break;

	case 104:
		{
		const MCCIADK_GUID_WIRE m104Guid =
#if defined(ARDUINO_MCCI_CATENA_4610)
			GUID_HW_CATENA_4610_M104(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4611)
			GUID_HW_CATENA_4611_M104(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4612)
			GUID_HW_CATENA_4612_M104(WIRE);
#else
# error not Catena 461x supported
#endif
		assertEqual(
			memcmp(&m104Guid, &pPlatform->Guid, sizeof(m104Guid)), 0,
			"platform guid mismatch"
			);
		}
		break;

	default:
		{
		const MCCIADK_GUID_WIRE baseGuid =
#if defined(ARDUINO_MCCI_CATENA_4610)
			GUID_HW_CATENA_4610_BASE(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4611)
			GUID_HW_CATENA_4611_BASE(WIRE);
#elif defined(ARDUINO_MCCI_CATENA_4612)
			GUID_HW_CATENA_4612_BASE(WIRE);
#else
# error not Catena 461x supported
#endif
		assertEqual(
			memcmp(&baseGuid, &pPlatform->Guid, sizeof(baseGuid)), 0,
			"platform guid mismatch"
			);
		}
		break;
		}
	}

test(2_platform_10_check_syseui)
	{
	const Catena::EUI64_buffer_t *pSysEUI = gCatena.GetSysEUI();
	static const Catena::EUI64_buffer_t ZeroEUI = { 0 };
	bool fNonZeroEUI;

	assertTrue(pSysEUI != nullptr);
	fNonZeroEUI = memcmp(pSysEUI, &ZeroEUI, sizeof(ZeroEUI)) != 0;
	assertTrue(fNonZeroEUI, "SysEUI is zero. This is normal on first boot.");

	for (unsigned i = 0; i < sizeof(pSysEUI->b); ++i)
		{
		gCatena.SafePrintf("%s%02x", i == 0 ? "  SysEUI: " : "-", pSysEUI->b[i]);
		}
	gCatena.SafePrintf("\n");
	}

test(2_platform_20_lorawan_begin)
	{
	bool fPassed = gLoRaWAN.begin(&gCatena);

	// always register
	gCatena.registerObject(&gLoRaWAN);

	assertTrue(fPassed, "gLoRaWAN.begin() failed");
	}

test(2_platform_30_init_lux)
	{
	uint32_t flags = gCatena.GetPlatformFlags();
	bool fLuxFound;

	assertNotEqual(flags & Catena::fHasLuxSi1113, 0, "No lux sensor in platform flags?");

	if (gSi1133.begin())
		{
		fLuxFound = true;
		gSi1133.configure(0, CATENA_SI1133_MODE_SmallIR);
		gSi1133.configure(1, CATENA_SI1133_MODE_White);
		gSi1133.configure(2, CATENA_SI1133_MODE_UV);
		gSi1133.start();
		}
	else
		{
		fLuxFound = false;
		}
	assertTrue(fLuxFound, "Si1133 sensor failed begin()");
	}

test(2_platform_40_init_bme)
	{
	uint32_t flags = gCatena.GetPlatformFlags();

	assertNotEqual(flags & Catena::fHasBme280, 0, "No BME280 sensor in platform flags?");
	assertTrue(
		gBME280.begin(BME280_ADDRESS, Adafruit_BME280::OPERATING_MODE::Sleep),
		"BME280 sensor failed begin(): check wiring"
		);
	}

testing(3_platform_50_lux)
	{
	assertTestPass(2_platform_30_init_lux);

	const uint32_t interval = 2000;
	const uint32_t ntries = 10;
	static uint32_t lasttime, thistry;
	uint32_t now = millis();

	if ((int32_t)(now - lasttime) < interval)
		/* skip */;
	else
		{
		lasttime = now;

		/* Get a new sensor event */
		uint16_t data[3];

		gSi1133.readMultiChannelData(data, 3);
		assertLess(data[1], 0xFFFF / 1.2, "Oops: light value pegged: " << data[1]);
		gCatena.SafePrintf(
			"Si1133:  %u IR, %u White, %u UV\n",
			data[0],
			data[1],
			data[2]
			);

		if (++thistry >= ntries)
			pass();
		}
	}

testing(3_platform_60_bme)
	{
	assertTestPass(2_platform_40_init_bme);

	const uint32_t interval = 2000;
	const uint32_t ntries = 10;
	static uint32_t lasttime, thistry;
	uint32_t now = millis();

	if ((int32_t)(now - lasttime) < interval)
		/* skip */;
	else
		{
		lasttime = now;

		Adafruit_BME280::Measurements m = gBME280.readTemperaturePressureHumidity();
		Serial.print("BME280:  T: "); Serial.print(m.Temperature);
		Serial.print("  P: "); Serial.print(m.Pressure);
		Serial.print("  RH: "); Serial.print(m.Humidity); Serial.println("%");

		assertMore(m.Temperature, 10, "Temperature in lab must be > 10 deg C: " << m.Temperature);
		assertLess(m.Temperature, 40, "Temperature in lab must be < 40 deg C: " << m.Temperature);
		assertMore(m.Pressure, 800 * 100);
		assertLess(m.Pressure, 1100 * 100);
		assertMore(m.Humidity, 5);
		assertLess(m.Humidity, 100);

		if (++thistry >= ntries)
			pass();
		}
	}

testing(3_platform_70_vBat)
	{
	const uint32_t interval = 2000;
	const uint32_t ntries = 10;
	static uint32_t lasttime, thistry;
	uint32_t now = millis();

	if ((int32_t)(now - lasttime) < interval)
		/* skip */;
	else
		{
		lasttime = now;

		float vBat = gCatena.ReadVbat();
		Serial.print("Vbat:   "); Serial.println(vBat);

		float vBus = gCatena.ReadVbus();
		Serial.print("Vbus:   "); Serial.println(vBus);

		assertMore(vBat, 1.4);
		assertLess(vBat, 4.5);

		assertMore(vBus, 1.0);
		assertLess(vBus, 4.5);

		if (++thistry >= ntries)
			pass();
		}
	}

testing(3_platform_99)
	{
	if (checkTestDone(3_platform_50_lux) &&
	    checkTestDone(3_platform_60_bme) &&
	    checkTestDone(3_platform_70_vBat))
		{
		assertTestPass(3_platform_50_lux);
		assertTestPass(3_platform_60_bme);
		assertTestPass(3_platform_70_vBat);
		pass();
		}
	}

//-----------------------------------------------------
//      Network tests
//-----------------------------------------------------

// make sure we're provisioned.
testing(4_lora_00_provisioned)
	{
	if (! checkTestDone(3_platform_99))
		return;

	assertTrue(gLoRaWAN.IsProvisioned(), "Not provisioned yet");
	pass();
	}

// Send a confirmed uplink message
Arduino_LoRaWAN::SendBufferCbFn uplinkDone;

uint8_t noncePointer;
bool gfSuccess;
bool gfTxDone;
void *gpCtx;

uint8_t uplinkBuffer[] = { /* port */ 0x10, 0xCA, 0xFE, 0xBA,0xBE };

const uint32_t kLoRaSendTimeout = 40 * 1000;

uint32_t gTxStartTime;

testing(4_lora_10_senduplink)
	{
	if (! checkTestDone(4_lora_00_provisioned))
		return;

	// send a confirmed message.
	if (! checkTestPass(4_lora_00_provisioned))
		{
		skip();
		return;
		}

	assertTrue(gLoRaWAN.SendBuffer(uplinkBuffer, sizeof(uplinkBuffer), uplinkDone, (void *) &noncePointer, true), "SendBuffer failed");
	gTxStartTime = millis();
	pass();
	}

void uplinkDone(void *pCtx, bool fSuccess)
	{
	gfTxDone = true;
	gfSuccess = fSuccess;
	gpCtx = pCtx;
	}

testing(4_lora_20_uplink_done)
	{
	if (checkTestSkip(4_lora_10_senduplink))
		{
		skip();
		return;
		}

	if (!checkTestDone(4_lora_10_senduplink))
		return;

	if (!checkTestPass(4_lora_10_senduplink))
		{
		skip();
		return;
		}

	if (! gfTxDone)
		{
		assertLess((int32_t)(millis() - gTxStartTime), kLoRaSendTimeout, "LoRa transmit timed out");

		return;
		}

	assertTrue(gfSuccess, "Message uplink failed");
	assertTrue(gpCtx == (void *)&noncePointer, "Context pointer was wrong on callback");
	pass();
	fHardwareTestDone = true;
	}


//-----------------------------------------------------
//      Flash tests
//-----------------------------------------------------
void setup_flash(void)
	{
	gSPI2.begin();
	}

void logMismatch(
	uint32_t addr,
	uint8_t expect,
	uint8_t actual
	)
	{
	gCatena.SafePrintf(
		"mismatch address %#x: expect %#02x got %02x\n",
		addr, expect, actual
		);
	}

uint32_t vNext(uint32_t v)
	{
	return v * 31413 + 3;
	}

// choose a sector.
const uint32_t sectorAddress = MX25V8035F_SECTOR_SIZE * 15;

union sectorBuffer_t {
	uint8_t b[MX25V8035F_SECTOR_SIZE];
	uint32_t dw[MX25V8035F_SECTOR_SIZE / sizeof(uint32_t)];
	} sectorBuffer;

test(1_flash_00init)
	{
	assertTrue(flash_init(), "flash_init()");
	pass();
	}

test(1_flash_01erase)
	{
	// erase the sector.
	assertTestPass(1_flash_00init);
	gFlash.eraseSector(sectorAddress);
	pass();
	}

uint32_t flashBlankCheck(
	uint32_t a = sectorAddress,
	sectorBuffer_t &buffer = sectorBuffer
	)
	{
	// make sure the sector is blank
	memset(buffer.b, 0, sizeof(buffer));
	gFlash.read(a, buffer.b, sizeof(buffer.b));
	unsigned errs = 0;
	for (auto i = 0; i < sizeof(buffer.b); ++i)
		{
		if (buffer.b[i] != 0xFF)
			{
			logMismatch(a + i, 0xFF, buffer.b[i]);
			++errs;
			}
		}
	return errs;
	}

test(1_flash_02blankcheck)
	{
	auto errs = flashBlankCheck();

	assertEqual(errs, 0, "mismatch errors: " << errs);
	pass();
	}

void initBuffer(
	uint32_t v,
	sectorBuffer_t &buffer = sectorBuffer
	)
	{
	for (auto i = 0;
	     i < sizeof(buffer.dw) / sizeof(buffer.dw[0]);
	     ++i, v = vNext(v))
		{
		buffer.dw[i] = v;
		}
	}

const uint32_t vStart = 0x55555555u;

test(1_flash_03writepattern)
	{
	// write a pattern
	initBuffer(vStart, sectorBuffer);

	gFlash.program(sectorAddress, sectorBuffer.b, sizeof(sectorBuffer.b));
	pass();
	}

test(1_flash_04readpattern)
	{
	// read the buffer
	for (auto i = 0; i < sizeof(sectorBuffer.b); ++i)
		sectorBuffer.b[i] = ~sectorBuffer.b[i];

	gFlash.read(sectorAddress, sectorBuffer.b, sizeof(sectorBuffer.b));

	union 	{
		uint8_t b[sizeof(uint32_t)];
		uint32_t dw;
		} vTest, v;
	v.dw = vStart;

	auto errs = 0;
	for (auto i = 0;
	     i < sizeof(sectorBuffer.dw) / sizeof(sectorBuffer.dw[0]);
	     ++i, v.dw = vNext(v.dw))
		{
		vTest.dw = sectorBuffer.dw[i];

		for (auto j = 0; j < sizeof(v.b); ++j)
			{
			if (v.b[j] != vTest.b[j])
				{
				++errs;
				logMismatch(sectorAddress + sizeof(uint32_t) * i + j, v.b[j], vTest.b[j]);
				}
			}
		}
	assertEqual(errs, 0, "mismatch errors: " << errs);
	pass();
	}

test(1_flash_05posterase)
	{
	gFlash.eraseSector(sectorAddress);

	auto errs = flashBlankCheck();

	assertEqual(errs, 0, "mismatch errors: " << errs);
	pass();
	}

testing(1_flash_99done)
	{
	if (checkTestDone(1_flash_05posterase))
		{
		assertTestPass(1_flash_05posterase);
		pass();
		}
	gFlash.powerDown();
	}

// boilerplate setup code
bool flash_init(void)
	{
	bool fFlashFound;

	gCatena.SafePrintf("Init FLASH\n");

	if (gFlash.begin(&gSPI2, Catena::PIN_SPI2_FLASH_SS))
		{
		uint8_t ManufacturerId;
		uint16_t DeviceId;

		gFlash.readId(&ManufacturerId, &DeviceId);
		gCatena.SafePrintf(
			"FLASH found, ManufacturerId=%02x, DeviceId=%04x\n",
			ManufacturerId, DeviceId
			);
		fFlashFound = true;
		}
	else
		{
		gFlash.end();
		gSPI2.end();
		gCatena.SafePrintf("No FLASH found\n");
		fFlashFound = false;
		}

	return fFlashFound;
	}

//-------------------------------------------------
//      Sensor tests
//-------------------------------------------------

void adc_testing(void)
	{
	const uint32_t interval = 2000;
	static uint32_t lasttime;
	uint32_t now = millis();

	if ((int32_t)(now - lasttime) < interval)
		/* skip */;
	else
		{
		lasttime = now;

		uint32_t vBat = analogRead(Catena461x::APIN_VBAT_SENSE);;
		Serial.print("\nvBat:   "); Serial.println(vBat);

		uint32_t vBus = analogRead(Catena461x::APIN_VBUS_SENSE);
		Serial.print("vBus:   "); Serial.println(vBus);

		uint32_t vA0 = analogRead(A0);
		Serial.print("vA0:    "); Serial.println(vA0);

		uint32_t vA1 = analogRead(A1);
		Serial.print("vA1:    "); Serial.println(vA1);

#if NUM_DIGITAL_PINS > 34
		uint32_t vRef = analogRead(D34);
		Serial.print("vRef:   "); Serial.println(vRef);
#endif
		}
	}

/**** end of catena461x_hwtest.ino ****/
