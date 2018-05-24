#include <Catena4460.h>
#include <Catena_Led.h>
#include "bsec.h"
#include <cstring>
#include "mcciadk_baselib.h"

using namespace McciCatena;

extern const uint8_t bsec_config_iaq[];
//#include "bsec_serialized_configurations_iaq.h"

#define STATE_SAVE_PERIOD  UINT32_C(360 * 60 * 1000) // 360 minutes - 4 times a day

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);
void loadState(void);
void updateState(void);

// the Catena object
using Catena = Catena4460;

Catena gCatena;
// declare the LED object
StatusLed gLed (Catena::PIN_STATUS_LED);

// Create an object of the class Bsec
Bsec iaqSensor;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;
uint32_t millisOverflowCounter = 0;
uint32_t lastTime = 0;

String output;

// Entry point for the example
void setup(void)
{
  gCatena.begin();
  gLed.begin();
  gCatena.registerObject(&gLed);
  gLed.Set(LedPattern::FastFlash);

  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  iaqSensor.setConfig(bsec_config_iaq);
  checkIaqSensorStatus();

  loadState();

  bsec_virtual_sensor_t sensorList[7] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ_ESTIMATE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 7, BSEC_SAMPLE_RATE_ULP);
  checkIaqSensorStatus();

  // Print the header
  output = "Timestamp [ms], raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%]";
  Serial.println(output);
}

// Function that is looped forever
void loop(void)
{
  gCatena.poll();

  if (iaqSensor.run()) { // If new data is available
    output = String(millis());
    output += ", " + String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.pressure);
    output += ", " + String(iaqSensor.rawHumidity);
    output += ", " + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.iaqEstimate);
    output += ", " + String(iaqSensor.iaqAccuracy);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    Serial.println(output);
    updateState();
  } else {
    checkIaqSensorStatus();
  }
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

void errLeds(void)
{
  gLed.Set(LedPattern::ThreeShort);

  for (;;)
     gCatena.poll();
}

void dumpState(void)
{
    for (auto here = 0, nLeft = BSEC_MAX_STATE_BLOB_SIZE; 
         nLeft != 0; 
	)
	{
    	char line[80];
    	size_t n;

	n = 16;
	if (n >= nLeft)
		n = nLeft;

	McciAdkLib_FormatDumpLine(
		line, sizeof(line), 0,  
		here, 
		bsecState + here, n
		);

	Serial.println(line);

	here += n, nLeft -= n;
	}
}

void loadState(void)
{
  cFram * const pFram = gCatena.getFram();

  if (pFram->getField(cFramStorage::kBme680Cal, bsecState)) {
    Serial.println("Got state from FRAM:");
    dumpState();

    iaqSensor.setState(bsecState);
    checkIaqSensorStatus();
  } else {
    // zero the saved state
    Serial.println("Zeroing state");

    std::memset(bsecState, 0, sizeof(bsecState));
    pFram->saveField(cFramStorage::kBme680Cal, bsecState);
  }
}

// time stamp of last time we 
uint32_t lastUpdate = 0;

void updateState(void)
{
  bool update = false;
  if (stateUpdateCounter == 0) {
    /* First state update when IAQ accuracy is >= 1 */
    if (iaqSensor.iaqAccuracy >= 3) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    /* Update every STATE_SAVE_PERIOD minutes */
    if ((int32_t)(millis() - lastUpdate) >= STATE_SAVE_PERIOD) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    lastUpdate = millis();
    iaqSensor.getState(bsecState);
    checkIaqSensorStatus();

    Serial.println("Writing state to FRAM");
    gCatena.getFram()->saveField(cFramStorage::kBme680Cal, bsecState);
    dumpState();
  }
}

#if 0
/*!
   @brief       Interrupt handler for press of a ULP plus button

   @return      none
*/
void ulp_plus_button_press()
{
  /* We call bsec_update_subscription() in order to instruct BSEC to perform an extra measurement at the next
     possible time slot
  */
  Serial.println("Triggering ULP plus.");
  bsec_virtual_sensor_t sensorList[1] = {
    BSEC_OUTPUT_IAQ_ESTIMATE,
  };

  iaqSensor.updateSubscription(sensorList, 1, BSEC_SAMPLE_RATE_ULP_MEASUREMENT_ON_DEMAND);
  checkIaqSensorStatus();
}
#endif
