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

#include <type_traits>

/****************************************************************************\
|
|		Manifest constants & typedefs.
|
|	This is strictly for private types and constants which will not 
|	be exported.
|
\****************************************************************************/

// each bit is 128 ms
enum class LedPattern:uint64_t
        {
        Off = 0,
        On = 1,

        OneEigth = 0b100000001,
        OneSixteenth = 0b10000000000000001,
        TwoShort = 0b10000000000000001001,
        FiftyFiftySlow = 0b100000000000000001111111111111111,

        Joining = TwoShort,
        Sending = FiftyFiftySlow,
        };

class StatusLed
        {
private:
        typedef std::underlying_type<LedPattern>::type LedPatternInt;

public:
        StatusLed(uint8_t uPin) : m_Pin(uPin) {};
        void begin(void)
                {
                pinMode(m_Pin, OUTPUT);
                digitalWrite(m_Pin, LOW);
                m_StartTime = millis();
                m_Pattern = LedPattern::Off;
                }
        
        void loop(void)
                {
                uint32_t delta;
                uint32_t ticks;
                uint32_t now;

                now = millis();
                delta = now ^ m_StartTime;
                if (delta & (1 << 7))
                        {
                        // the 2^7 bit changes every 2^6 ms.
                        m_StartTime = now;

                        this->update();
                        }
                }

        void update(void)
                {
                if (m_Pattern == LedPattern::Off)
                        {
                        digitalWrite(m_Pin, LOW);
                        return;
                        }

                if (m_Current <= 1)
                        m_Current = static_cast<LedPatternInt>(m_Pattern);

                digitalWrite(m_Pin, m_Current & 1);
                m_Current >>= 1;
                }

        LedPattern Set(LedPattern newPattern)
                {
                LedPattern const oldPattern = m_Pattern;

                m_Pattern = newPattern;
                m_Current = 0;
                digitalWrite(m_Pin, LOW);
                }

private:
        uint8_t         m_Pin;
        LedPattern      m_Pattern;
        LedPatternInt   m_Current;
        uint32_t        m_StartTime;
        };

// forwards
static void configureLuxSensor(void);
static void displayLuxSensorDetails(void);
static bool displayTempSensorDetails(void);

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
Catena4410 gCatena4410;

//
// the LoRaWAN backhaul.  Note that we use the
// Catena4410 version so it can provide hardware-specific
// information to the base class.
//
Catena4410::LoRaWAN gLoRaWAN;

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
    gCatena4410.begin();

    gCatena4410.SafePrintf("Catena 4410 sensor1 %s %s\n", __DATE__, __TIME__);

    // set up the status led
    gLed.begin();

    if (! gLoRaWAN.begin(&gCatena4410))
	gCatena4410.SafePrintf("LoRaWAN init failed\n");

    Catena4410::UniqueID_string_t CpuIDstring;

    gCatena4410.SafePrintf("CPU Unique ID: %s\n",
        gCatena4410.GetUniqueIDstring(&CpuIDstring)
        );

    /* find the platform */
    const Catena4410::EUI64_buffer_t *pSysEUI = gCatena4410.GetSysEUI();

    const CATENA_PLATFORM * const pPlatform = gCatena4410.GetPlatform();

    if (pPlatform)
    {
      gCatena4410.SafePrintf("EUI64: ");
      for (unsigned i = 0; i < sizeof(pSysEUI->b); ++i)
      {
        gCatena4410.SafePrintf("%s%02x", i == 0 ? "" : "-", pSysEUI->b[i]);
      }
      gCatena4410.SafePrintf("\n");
      gCatena4410.SafePrintf(
            "Platform Flags:  %#010x\n",
            gCatena4410.GetPlatformFlags()
            );
      gCatena4410.SafePrintf(
            "Operating Flags:  %#010x\n",
            gCatena4410.GetOperatingFlags()
            );
    }

    /* initialize the lux sensor */
    if (! tsl.begin())
    {
      gCatena4410.SafePrintf("No TSL2561 detected: check wiring\n");
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
      gCatena4410.SafePrintf("No BME280 found: check wiring\n");
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
      gCatena4410.SafePrintf("water temperature not found: is it connected?\n");
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

    // startSendingUplink();
}

void loop() 
{
  gLoRaWAN.loop();

  gLed.loop();
}

void startSendingUplink(void)
{
  Adafruit_BME280::Measurements Measurements;

  if (fBme)
       {
       bme.readTemperaturePressureHumidity();
       }
}
#if 0
// we can't actually sucessfully send and run the sensors, so this app
  // doesn't try to send.
  if (gLoRaWAN.GetTxReady())
    {
//  const static uint8_t msg[] = "hello world";
//  gLoRaWAN.SendBuffer(msg, sizeof(msg) - 1); 
    }


  Serial.print("Vbat = "); Serial.print(gCatena4410.ReadVbat()); Serial.println(" V");
  if (fBme)
  {
     Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");
  }
  else
  {
    gCatena4410.SafePrintf("No BME280 sensor\n");
  }
  if (fTsl)
  {
    /* Get a new sensor event */ 
    sensors_event_t event;
    tsl.getEvent(&event);
   
    /* Display the results (light is measured in lux) */
    if (event.light)
    {
      Serial.print(event.light); Serial.println(" lux");
    }
    else
    {
      /* If event.light = 0 lux the sensor is probably saturated
         and no reliable data could be generated! */
      Serial.println("Sensor overload");
    }
  }
  else
  {
    gCatena4410.SafePrintf("No Lux sensor\n");
  }

  if (fWaterTemp)
  {
    sensor_WaterTemp.requestTemperatures();
    float waterTempC = sensor_WaterTemp.getTempCByIndex(0);
    Serial.print("Water temperature: "); Serial.print(waterTempC); Serial.println(" C");
  }
  else
  {
    gCatena4410.SafePrintf("No water temperature\n");
  }

  if (fSoilSensor)
  {
    /* display temp and RH. library doesn't tell whether sensor is disconnected but gives us huge values instead */
    Serial.print("Soil temperature: "); Serial.print(sensor_Soil.readTemperatureC()); Serial.println(" C");
    Serial.print("Soil humidity:    "); Serial.print(sensor_Soil.readHumidity()); Serial.println(" %");
  }
  delay(2000);
}
#endif

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


static bool displayTempSensorDetails(void)
{
  const unsigned nDevices = sensor_WaterTemp.getDeviceCount();
  if (nDevices == 0)
    return false;

  gCatena4410.SafePrintf("found %u devices\n", nDevices);
  for (unsigned iDevice = 0; iDevice < nDevices; ++iDevice)
  {
    // print interesting info
  }
  return true;
}
