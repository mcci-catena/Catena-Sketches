/* catena4450_test01.ino	Fri Mar 10 2017 18:51:56 tmm */

/*

Module:  catena4450_test01.ino

Function:
	Test program for Catena 4450 and variants.

Version:
	V1.00a	Fri Mar 10 2017 18:51:56 tmm	Edit level 1

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
   1.00a  Fri Mar 10 2017 18:51:56  tmm
	Module created.

*/

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <stdio.h>
#include <stdarg.h>
#include <Adafruit_FRAM_I2C.h>
#include <Catena4410.h>

/****************************************************************************\
|
|		Manifest constants & typedefs.
|
|	This is strictly for private types and constants which will not 
|	be exported.
|
\****************************************************************************/

//#define SEALEVELPRESSURE_HPA (1013.25)
#define SEALEVELPRESSURE_HPA (1027.087) // 3170 Perry City Road, 2016-10-04 22:57

static void configureLuxSensor(void);
static void displayLuxSensorDetails(void);


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
// the catena
Catena4410 gCatena;

//   The temperature/humidity sensor
Adafruit_BME280 bme; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
BH1750 bh1750;
bool fLux;
Adafruit_FRAM_I2C fram;
bool fFram;

//   The contact sensors
bool fHasPower1;
uint8_t kPinPower1P1;
uint8_t kPinPower1P2;

/*

Name:	setup()

Function:
	Arduino setup routine: called once at boot.

Definition:
	void setup(void);

Description:
	In this test program, setup does the required things for initializing
	the platform.

	This test is intended for simplistic stand-alone use.

Returns:
	No explicit result.

*/

void setup() 
{
    bool fNoGood;

    fNoGood = false;
    
    gCatena.begin();
    
    while (! Serial)
      /* idle */;

    gCatena.SafePrintf("Basic Catena 4450 test %s %s\n", __DATE__, __TIME__);
    CatenaSamd21::UniqueID_string_t CpuIDstring;

    gCatena.SafePrintf("CPU Unique ID: %s\n", 
      gCatena.GetUniqueIDstring(&CpuIDstring)
      );

    const CatenaBase::EUI64_buffer_t *pSysEUI = gCatena.GetSysEUI();
    static const CatenaBase::EUI64_buffer_t zeroEUI = {};

    if (pSysEUI == NULL)
      {
      gCatena.SafePrintf("**** no SysEUI, check provisioning ****\n");
      }
   else if (memcmp(pSysEUI, &zeroEUI, sizeof(zeroEUI)) == 0)
      {
      gCatena.SafePrintf("**** SysEUI == 0, check provisioning ****\n");
      fNoGood = true;
      }
   else 
      {
      gCatena.SafePrintf("EUI64: ");
      for (unsigned i = 0; i < sizeof(pSysEUI->b); ++i)
        {
        gCatena.SafePrintf("%s%02x", i == 0 ? "" : "-", pSysEUI->b[i]);
        }
      gCatena.SafePrintf("\n");
      fNoGood = true;
      }

    uint32_t flags;
    const CATENA_PLATFORM * const pPlatform = gCatena.GetPlatform();
    if (pPlatform == NULL)
      {
      gCatena.SafePrintf("**** no platform, check provisioning ****\n");
      fNoGood = true;
      flags = 0;
      }
    else
      {
      flags = gCatena.GetPlatformFlags();

      gCatena.SafePrintf(
          "Platform flags: 0x%08x\n",
          flags
          );
      }

    /* initialize the lux sensor */
    if (flags & CatenaSamd21::fHasLuxRohm)
      {
      bh1750.begin();
      fLux = true;
      displayLuxSensorDetails();
      }
    else
      {
      fLux = false;
      }

    /* initialize the BME280 */
    if (! (flags & CatenaSamd21::fHasBme280))
    {
      gCatena.SafePrintf("flags say no BME280!\n");
      fBme = false;
    }
    else if (! bme.begin())
    {
      gCatena.SafePrintf("No BME280 found: check wiring\n");
      fBme = false;
    }
    else
    {
      /* read and discard one value */
      (void) bme.readTemperature();
      fBme = true;
    }

    /* initialize the FRAM */
    if (! (flags & CatenaSamd21::fHasFRAM))
      {
        gCatena.SafePrintf("flags say no FRAM!\n");
        fFram = false;
      }
    else if (! fram.begin())
      {
        gCatena.SafePrintf("failed to init FRAM: check wiring\n");
        fFram = false;
      }
    else
      {
        fFram = true;
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
        
        pinMode(kPinPower1P1, INPUT);
        pinMode(kPinPower1P2, INPUT);
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

Name:	loop()

Function:
	The Arduino runtime loop.

Definition:
	void loop(void);

Description:
	This routine is called continually from the background scheduler,
	and services all the operating tasks.

	In this test program, it simply prints information from the enabled
	sensors to the USB serial port.

Returns:
	No explicit result.

*/

void loop() 
{
  if (fBme)
  {
    if (Serial) Serial.print("Temperature = ");
    if (Serial) Serial.print(bme.readTemperature());
    if (Serial) Serial.println(" *C");

    if (Serial) Serial.print("Pressure = ");
    if (Serial) Serial.print(bme.readPressure() / 100.0F);
    if (Serial) Serial.println(" hPa");

    if (Serial) Serial.print("Approx. Altitude = ");
    if (Serial) Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    if (Serial) Serial.println(" m");

    if (Serial) Serial.print("Humidity = ");
    if (Serial) Serial.print(bme.readHumidity());
    if (Serial) Serial.println(" %");
  }
  else
  {
    gCatena.SafePrintf("No BME280 sensor\n");
  }
  if (fLux)
  {
    /* Get a new sensor event */ 
    uint16_t light;

    light = bh1750.readLightLevel();

    if (Serial) Serial.print(light); if (Serial) Serial.println(" lux");
  }
  else
  {
    gCatena.SafePrintf("No Lux sensor\n");
  }

  if (fFram)
  {
    uint32_t uCount;
    uint16_t pCount;

    pCount = 0;
    uCount = fram.read8(pCount+0) | fram.read8(pCount+1) << 8 | fram.read8(pCount+2) << 16 | fram.read8(pCount+3) << 24;
    if (Serial) Serial.print(uCount, HEX); if (Serial) Serial.println(" cycles");
    if (uCount == 0 || (uCount & -uCount) != uCount)
    {
      if (Serial) Serial.println("** not a power of 2, resetting **");
      uCount = 1;
    }
    uCount = uCount << 1;
    fram.write8(pCount + 0, uCount & 0xFF); 
    fram.write8(pCount + 1, (uCount >> 8) & 0xFF);
    fram.write8(pCount + 2, (uCount >> 16) & 0xFF);
    fram.write8(pCount + 3, (uCount >> 24) & 0xFF);
  }

  if (fHasPower1)
    {
    int p1Val, p2Val;

    p1Val = digitalRead(kPinPower1P1);
    p2Val = digitalRead(kPinPower1P2);

    gCatena.SafePrintf("P1 = %-4s  P2 = %-4s\n", 
        p1Val ? "HIGH": "low", 
        p2Val ? "HIGH" : "low"
        );
    }

  gCatena.SafePrintf("---\n");
  delay(2000);
}

/* functions */
static void displayLuxSensorDetails(void)
{
#if 0
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
#endif
}

static void configureLuxSensor(void)
{
  bh1750.configure(BH1750_ONE_TIME_HIGH_RES_MODE_2);
}
