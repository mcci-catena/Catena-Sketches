#include <Wire.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <stdio.h>
#include <stdarg.h>
#include <Adafruit_FRAM_I2C.h>
#include <Catena4410.h>

// manifests.
//#define SEALEVELPRESSURE_HPA (1013.25)
#define SEALEVELPRESSURE_HPA (1027.087) // 3170 Perry City Road, 2016-10-04 22:57

static void configureLuxSensor(void);
static void displayLuxSensorDetails(void);

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

void setup() 
{
    gCatena.begin();

    gCatena.SafePrintf("Basic Catena 4450 test %s %s\n", __DATE__, __TIME__);
    CatenaSamd21::UniqueID_string_t CpuIDstring;

    gCatena.SafePrintf("CPU Unique ID: %s\n", 
      gCatena.GetUniqueIDstring(&CpuIDstring)
      );

    /* initialize the lux sensor */
    bh1750.begin();
    fLux = true;
    displayLuxSensorDetails();

    /* initialize the BME280 */
    if (! bme.begin())
    {
      gCatena.SafePrintf("No BME280 found: check wiring\n");
      fBme = false;
    }
    else
    {
      fBme = true;
    }

    /* initialize the FRAM */
    if (! fram.begin())
      {
        gCatena.SafePrintf("failed to init FRAM: check wiring\n");
        fFram = false;
      }
    else
      {
        fFram = true;
      }

}

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

