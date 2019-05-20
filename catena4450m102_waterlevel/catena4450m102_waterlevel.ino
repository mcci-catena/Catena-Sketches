/* catena4450m102_waterlevel.ino	Sat Feb 03 2018 16:11:23 vel */

/*

Module:  catena4450m102_waterlevel.ino

Function:
	Code for the Water pressure transducer with Catena 4450-M102.

Version:
	V0.2.0	Sat Feb 03 2018 16:11:23 vel	Edit level 4

Copyright notice:
	This file copyright (C) 2017-18 by

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

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SHT1x.h>

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

/* how long do we wait between measurements (in seconds) */
enum    {
        // set this to interval between measurements, in seconds
        // Actual time will be a little longer because have to
        // add measurement and broadcast time.
        CATCFG_T_CYCLE = 6 * 60, // ten messages/hour
        CATCFG_T_WARMUP = 1,
        CATCFG_T_SETTLE = 5,
        CATCFG_T_INTERVAL = CATCFG_T_CYCLE - (CATCFG_T_WARMUP +
                                                CATCFG_T_SETTLE),
        };

enum    {
        PIN_ONE_WIRE =  A2,        // XSDA1 == A2
        PIN_SHT10_CLK = A1,        // XSCL0 == A1
        PIN_SHT10_DATA = A0,       // XSDA0 == A0
        };

// forwards
static void settleDoneCb(osjob_t *pSendJob);
static void warmupDoneCb(osjob_t *pSendJob);
static void txNotProvisionedCb(osjob_t *pSendJob);
static void sleepDoneCb(osjob_t *pSendJob);
static Arduino_LoRaWAN::SendBufferCbFn sendBufferDoneCb;
void fillBuffer(TxBuffer_t &b);
void startSendingUplink(void);

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
Adafruit_BME280 bme; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
BH1750 bh1750;
bool fLux;

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
		Wire.begin();
        gCatena.begin();

        gCatena.SafePrintf("Catena 4450 sensor1 V%s\n", sVersion);

        gLed.begin();
        gCatena.registerObject(&gLed);

#ifdef ARDUINO_ARCH_SAMD
        // set up the RTC object
        gRtc.begin();
#endif

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
        if (flags & CatenaBase::fHasLuxRohm)
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

        /* now, we kick off things by sending our first message */
        gLed.Set(LedPattern::Joining);

        /* warm up the BME280 by discarding a measurement */
        if (fBme)
                (void)bme.readTemperature();

        /* for 4551, we need wider tolerances, it seems */
#if defined(ARDUINO_ARCH_STM32)
        LMIC_setClockError(10 * 65536 / 100);
#endif

        /* trigger a join by sending the first packet */
        if (! (gCatena.GetOperatingFlags() &
                        static_cast<uint32_t>(gCatena.OPERATING_FLAGS::fManufacturingTest)))
                startSendingUplink();
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

       flag |= FlagsSensor4::FlagTPH;
       }

  if (fLux)
        {
        /* Get a new sensor event */
        uint16_t light;

        light = bh1750.readLightLevel();
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

  gLoRaWAN.SendBuffer(b.getbase(), b.getn(), sendBufferDoneCb, NULL);
}

static void sendBufferDoneCb(
    void *pContext,
    bool fStatus
    )
    {
    osjobcb_t pFn;

    gLed.Set(LedPattern::Settling);

    if (!fStatus)
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
            os_getTime()+sec2osticks(CATCFG_T_SETTLE),
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
    uint32_t startTime;
#ifdef ARDUINO_ARCH_SAMD
    const bool fDontSleep = true;
#else 
    const bool fDontSleep = Serial.dtr() || fHasPower1;
#endif /* ARDUINO_ARCH_SAMD */

    // if connected to USB, don't sleep
    // ditto if we're monitoring pulses.
    if (fDontSleep)
        {
        gLed.Set(LedPattern::Sleeping);
        os_setTimedCallback(
                &sensorJob,
                os_getTime() + sec2osticks(CATCFG_T_INTERVAL),
                sleepDoneCb
                );
        return;
        }

#ifdef ARDUINO_ARCH_SAMD
    /* ok... now it's time for a deep sleep */
    gLed.Set(LedPattern::Off);

    startTime = millis();
    gRtc.SetAlarm(CATCFG_T_INTERVAL);
    gRtc.SleepForAlarm(
        gRtc.MATCH_HHMMSS,
        gRtc.SleepMode::IdleCpuAhbApb
        );

    // add the number of ms that we were asleep to the millisecond timer.
    // we don't need extreme accuracy.
    adjust_millis_forward(CATCFG_T_INTERVAL  * 1000);

    /* and now... we're awake again. trigger another measurement */
    sleepDoneCb(pSendJob);
#endif /* ARDUINO_ARCH_SAMD */

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
