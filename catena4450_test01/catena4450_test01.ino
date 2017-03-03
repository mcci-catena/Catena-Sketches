#include <Wire.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <stdio.h>
#include <stdarg.h>
#include <Adafruit_FRAM_I2C.h>

// manifests.
//#define SEALEVELPRESSURE_HPA (1013.25)
#define SEALEVELPRESSURE_HPA (1027.087) // 3170 Perry City Road, 2016-10-04 22:57

static void configureLuxSensor(void);
static void displayLuxSensorDetails(void);

// globals
//   The temperature/humidity sensor
Adafruit_BME280 bme; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
BH1750 bh1750;
bool fLux;
Adafruit_FRAM_I2C fram;
bool fFram;

void safe_printf(const char *fmt, ...)
{
    if (! Serial) 
      return;

    char buf[128];
    va_list ap;
    int nc;
    
    va_start(ap, fmt);
    nc = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    // in case we overflowed:
    buf[sizeof(buf) - 1] = '\0';
    Serial.print(buf);
}

void setup() 
{
    while (!Serial); // wait for Serial to be initialized
    Serial.begin(115200);

    safe_printf("Basic Catena 4450 test\n");

    /* initialize the lux sensor */
    bh1750.begin();
    fLux = true;
    displayLuxSensorDetails();

    /* initialize the BME280 */
    if (! bme.begin())
    {
      safe_printf("No BME280 found: check wiring\n");
      fBme = false;
    }
    else
    {
      fBme = true;
    }

    /* initialize the FRAM */
    if (! fram.begin())
      {
        safe_printf("failed to init FRAM: check wiring\n");
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
    safe_printf("No BME280 sensor\n");
  }
  if (fLux)
  {
    /* Get a new sensor event */ 
    uint16_t light;

    light = bh1750.readLightLevel();

    Serial.print(light); Serial.println(" lux");
  }
  else
  {
    safe_printf("No Lux sensor\n");
  }

  if (fFram)
  {
    uint32_t uCount;
    uint16_t pCount;

    pCount = 0;
    uCount = fram.read8(pCount+0) | fram.read8(pCount+1) << 8 | fram.read8(pCount+2) << 16 | fram.read8(pCount+3) << 24;
    Serial.print(uCount, HEX); Serial.println(" cycles");
    if (uCount == 0 || (uCount & -uCount) != uCount)
    {
      Serial.println("** not a power of 2, resetting **");
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

