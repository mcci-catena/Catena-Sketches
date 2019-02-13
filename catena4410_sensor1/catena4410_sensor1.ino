/* catena4410_sensor1.ino	Tue Nov  1 2016 16:33:44 tmm */

/*

Module:  catena4410_sensor1.ino

Function:
	Standard LoRaWAN sensor configuration 1

Version:
	V0.1.0	Tue Nov  1 2016 16:33:44 tmm	Edit level 2

Copyright notice:
	This file copyright (C) 2016 by

		MCCI Corporation
		3520 Krums Corners Road
                Ithaca, NY  14850

	An unpublished work.  All rights reserved.

	This file is proprietary information, and may not be disclosed or
	copied without the prior permission of MCCI Corporation.

Author:
	Terry Moore, MCCI Corporation	November 2016

Revision history:
   V0.1.0  Tue Nov  1 2016 16:32:44  tmm
	Module created.

*/


#include <Catena4410.h>
#include <Catena_LED.h>
#include <Catena_TxBuffer.h>
#include <Catena_CommandStream.h>

#include <CatenaRTC.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_BME280.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SHT1x.h>
#include <Arduino_LoRaWAN.h>
#include <lmic.h>
#include <hal/hal.h>
#include <mcciadk_baselib.h>

#include <type_traits>

using namespace McciCatena;

/****************************************************************************\
|
|		Manifest constants & typedefs.
|
|	This is strictly for private types and constants which will not
|	be exported.
|
\****************************************************************************/


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
static void configureLuxSensor(void);
static void displayLuxSensorDetails(void);
static bool displayTempSensorDetails(void);
static bool checkWaterSensorPresent(void);
static void settleDoneCb(osjob_t *pSendJob);
static void warmupDoneCb(osjob_t *pSendJob);
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
Catena4410 gCatena;

//
// the LoRaWAN backhaul.  Note that we use the
// Catena4410 version so it can provide hardware-specific
// information to the base class.
//
Catena4410::LoRaWAN gLoRaWAN;

// the RTC instance, used for sleeping
CatenaRTC gRtc;

//   The temperature/humidity sensor
Adafruit_BME280 bme; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
bool fTsl;

//   The submersible temperature sensor
OneWire oneWire(Catena4410::PIN_ONE_WIRE);
DallasTemperature sensor_WaterTemp(&oneWire);
bool fWaterTemp;

//  The SHT10 soil sensor
SHT1x sensor_Soil(Catena4410::PIN_SHT10_DATA, Catena4410::PIN_SHT10_CLK);
bool fSoilSensor;

//  the LED flasher
StatusLed gLed(Catena4410::PIN_STATUS_LED);

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
        and (ultimately) return to the

Returns:
	No explicit result.

*/

void setup(void)
{
    gCatena.begin();

    gCatena.SafePrintf("Catena 4410 sensor1 V%s\n", sVersion);

    // set up the status led
    gLed.begin();

    // set up the RTC object
    gRtc.begin();

    if (! gLoRaWAN.begin(&gCatena))
	    gCatena.SafePrintf("LoRaWAN init failed\n");

    Catena4410::UniqueID_string_t CpuIDstring;

    gCatena.SafePrintf("CPU Unique ID: %s\n",
        gCatena.GetUniqueIDstring(&CpuIDstring)
        );

    /* find the platform */
    const Catena4410::EUI64_buffer_t *pSysEUI = gCatena.GetSysEUI();

    const CATENA_PLATFORM * const pPlatform = gCatena.GetPlatform();

    if (pPlatform)
    {
      gCatena.SafePrintf("EUI64: ");
      for (unsigned i = 0; i < sizeof(pSysEUI->b); ++i)
      {
        gCatena.SafePrintf("%s%02x", i == 0 ? "" : "-", pSysEUI->b[i]);
      }
      gCatena.SafePrintf("\n");
      gCatena.SafePrintf(
            "Platform Flags:  %#010x\n",
            gCatena.GetPlatformFlags()
            );
      gCatena.SafePrintf(
            "Operating Flags:  %#010x\n",
            gCatena.GetOperatingFlags()
            );
    }

    /* initialize the lux sensor */
    if (! tsl.begin())
    {
      gCatena.SafePrintf("No TSL2561 detected: check wiring\n");
      fTsl = false;
    }
    else
    {
      fTsl = true;
      displayLuxSensorDetails();
      configureLuxSensor();
    }

    /* initialize the BME280 */
    if (! bme.begin(BME280_ADDRESS, Adafruit_BME280::OPERATING_MODE::Sleep))
    {
      gCatena.SafePrintf("No BME280 found: check wiring\n");
      fBme = false;
    }
    else
    {
      fBme = true;
    }

    /* initialize the pond sensor */
    sensor_WaterTemp.begin();

    if (! displayTempSensorDetails())
    {
      gCatena.SafePrintf("water temperature not found: is it connected?\n");
      fWaterTemp = false;
    }
    else
    {
      fWaterTemp = true;
    }

    /* initalize the soil sensor... not yet.*/
    /* sensor_Soil.begin() */
    fSoilSensor = true;

    /* now, we kick off things by sending our first message */
    gLed.Set(LedPattern::Joining);

    /* warm up the BME280 by discarding a measurement */
    if (fBme)
       (void) bme.readTemperature();

    /* trigger a join by sending the first packet */
    startSendingUplink();

    /* set up the remaining pull-ups */
    uint32_t const uIoPullups = CATCFG_PULLUP_BITS;

    for (uint32_t i = 0; i < 32; ++i)
            {
            if (uIoPullups & (1 << i))
                PORT->Group[0].PINCFG[i].bit.PULLEN = 1;
            }
}

// The Arduino loop routine -- in our case, we just drive the other loops.
// If we try to do too much, we can break the LMIC radio. So the work is
// done by outcalls scheduled from the LMIC os loop.
void loop()
{
  gLoRaWAN.loop();

  gLed.loop();
}

void startSendingUplink(void)
{
  TxBuffer_t b;
  LedPattern savedLed = gLed.Set(LedPattern::Measuring);

  b.begin();
  uint8_t flag;

  flag = 0;

  b.put(FormatSensor1); /* the flag for this record format */
  uint8_t * const pFlag = b.getp();
  b.put(0x00); /* will be set to the flags */

  // vBat is sent as 5000 * v
  float vBat = gCatena.ReadVbat();
  gCatena.SafePrintf("vBat:    %d mV\n", (int) (vBat * 1000.0f));
  b.putV(vBat);
  flag |= FlagsSensor1::FlagVbat;

  if (fBme)
       {
       Adafruit_BME280::Measurements m = bme.readTemperaturePressureHumidity();
       // temperature is 2 bytes from -0x80.00 to +0x7F.FF degrees C
       // pressure is 2 bytes, hPa * 10.
       // humidity is one byte, where 0 == 0/256 and 0xFF == 255/256.
       gCatena.SafePrintf("BME280:  T: %d P: %d RH: %d\n", (int) m.Temperature, (int) m.Pressure, (int)m.Humidity);
       b.putT(m.Temperature);
       b.putP(m.Pressure);
       b.putRH(m.Humidity);

       flag |= FlagsSensor1::FlagTPH;
       }

  if (fTsl)
  {
    /* Get a new sensor event */
    sensors_event_t event;
    tsl.getEvent(&event);

    gCatena.SafePrintf("TSL:     Lux: %d\n", (int) event.light);

    /* record the results (light is measured in lux) */
    if (event.light > 0 && event.light < 65536)
    {
      b.putLux(event.light);
      flag |= FlagsSensor1::FlagLux;
    }
  }

  /*
  || Measure and transmit the "water temperature" (OneWire)
  || tranducer value. This is complicated because we want
  || to support plug/unplug and the sw interface is not
  || really hot-pluggable.
  */
  if (! fWaterTemp && checkWaterSensorPresent())
        fWaterTemp = true;

  if (fWaterTemp)
        {
        sensor_WaterTemp.requestTemperatures();
        float waterTempC = sensor_WaterTemp.getTempCByIndex(0);

        gCatena.SafePrintf("Water:   T: %d\n", (int) waterTempC);
        if (waterTempC <= -40.0)
                {
                // discard data and reset flag
                // so we'll check again next time
                fWaterTemp = false;
                }
        else
                {
                // transmit the measurement
                b.putT(waterTempC);
                flag |= FlagsSensor1::FlagWater;
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
        gCatena.SafePrintf("Soil:    T: %d RH: %d\n", (int) SoilT, (int) SoilRH);

        flag |= FlagsSensor1::FlagSoilTH;
        }
    }

  *pFlag = flag;
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
    gLed.Set(LedPattern::Settling);
    os_setTimedCallback(
            &sensorJob,
            os_getTime()+sec2osticks(CATCFG_T_SETTLE),
            settleDoneCb
            );
    }

#include <delay.h>

static void settleDoneCb(
    osjob_t *pSendJob
    )
    {
    uint32_t startTime;

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
    gLed.Set(LedPattern::WarmingUp);

    os_setTimedCallback(
            &sensorJob,
            os_getTime()+sec2osticks(CATCFG_T_WARMUP),
            warmupDoneCb
            );
    }

static void warmupDoneCb(
    osjob_t *pJob
    )
    {
    startSendingUplink();
    }

/* functions */
static void displayLuxSensorDetails(void)
{
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
}

static void configureLuxSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
}

static bool checkWaterSensorPresent(void)
{
  // this is unpleasant. But the way to deal with plugging is to call
  // begin again.
  sensor_WaterTemp.begin();
  return sensor_WaterTemp.getDeviceCount() != 0;
}

static bool displayTempSensorDetails(void)
{
  const unsigned nDevices = sensor_WaterTemp.getDeviceCount();
  if (nDevices == 0)
    return false;

  gCatena.SafePrintf("found %u devices\n", nDevices);
  for (unsigned iDevice = 0; iDevice < nDevices; ++iDevice)
  {
    // print interesting info
  }
  return true;
}
