#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_BME280.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <stdio.h>
#include <stdarg.h>

// manifests.
//#define SEALEVELPRESSURE_HPA (1013.25)
#define SEALEVELPRESSURE_HPA (1027.087) // 3170 Perry City Road, 2016-10-04 22:57

// +temp
#define PIN_ONE_WIRE  0 /* arduino D0 */
// -temp

// forwards
static void configureLuxSensor(void);
static void displayLuxSensorDetails(void);
static bool displayTempSensorDetails(void);

// globals
//   The temperature/humidity sensor
Adafruit_BME280 bme; // The default initalizer creates an I2C connection
bool fBme;

//   The LUX sensor
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
bool fTsl;

//   The submersible temperature sensor
OneWire oneWire(PIN_ONE_WIRE);
DallasTemperature sensor_WaterTemp(&oneWire);
bool fWaterTemp;
// -temp

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

    safe_printf("Basic Catena 4410 test\n");

    /* initialize the lux sensor */
    if (! tsl.begin())
    {
      safe_printf("No TSL2561 detected: check wiring\n");
      fTsl = false;
    }
    else
    {
      fTsl = true;
      displayLuxSensorDetails();
      configureLuxSensor();
    }

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

    /* initialize the pond sensor */
    sensor_WaterTemp.begin();

     if (! displayTempSensorDetails())
    {
      safe_printf("water temperature not found: is it connected?\n");
      fWaterTemp = false;
    }
    else
    {
      fWaterTemp = true;
    }
}

void loop() 
{
  DeviceAddress address;
  unsigned fSearch;
  safe_printf("oneWire.reset() ==> %u\n", oneWire.reset());
  fSearch = oneWire.search(address);
  if (fSearch)
  {
    safe_printf("oneWire.search() returned %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          address[0], address[1], address[2], address[3],
          address[4], address[5], address[6], address[7]
          );
    safe_printf("sensor_WaterTemp.validAddress() ==> %u\n", sensor_WaterTemp.validAddress(address) ? 1 : 0);
    safe_printf("sensor_WaterTemp.validFamily() ==> %u\n",  sensor_WaterTemp.validFamily(address) ? 1 : 0);
  }
  else
    safe_printf("oneWire.search() returned FALSE\n");

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
    safe_printf("No Lux sensor\n");
  }
  delay(2000);
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


static bool displayTempSensorDetails(void)
{
  const unsigned nDevices = sensor_WaterTemp.getDeviceCount();
  if (nDevices == 0)
    return false;

  for (unsigned iDevice = 0; iDevice < nDevices; ++iDevice)
  {
    // print interesting info
  }
}
