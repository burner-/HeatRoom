//float temp_main = READ_ERROR_VALUE;           //last temperature
float temp_backup = READ_ERROR_VALUE;           //last temperature

// current temperatures
float pidSensor = READ_ERROR_VALUE; //Sensor temp for hot water pid controlling
float hotWaterSensor = READ_ERROR_VALUE; //Sensor temp for hot water heating request
float coolingStartSensor = READ_ERROR_VALUE; //Sensor temp for cooling start
float coolingStopSensor = READ_ERROR_VALUE; //Sensor temp for cooling stop
float warmWaterStartSensor = READ_ERROR_VALUE; //Sensor temp for warming start
float warmWaterStopSensor = READ_ERROR_VALUE; //Sensor temp for warming stop
float compressorHotLimitSensor = READ_ERROR_VALUE; //Sensor temp for emergency stop
float compressorColdLimitSensor = READ_ERROR_VALUE; //Sensor temp for emergency stop
float bedroomTempSensor = READ_ERROR_VALUE; // Current temp for bedroom sensor

class sensorMapping {
  public:
    sensorMapping(String sensorName, float* TempVariable);
    byte sensorAddr[8] = {};
    float* Temperature;
    String SensorName;
};

sensorMapping::sensorMapping(String sensorName, float* TempVariable) {
  Temperature = TempVariable;
  SensorName = sensorName;
}

#define ARR_SENSORS 10
sensorMapping SensorMap[ARR_SENSORS] = {
  sensorMapping("backup", &temp_backup),
  sensorMapping("coolingStopSensor", &coolingStopSensor),
  sensorMapping("coolingStartSensor", &coolingStartSensor),
  sensorMapping("warmWaterStartSensor", &warmWaterStartSensor),
  sensorMapping("warmWaterStopSensor", &warmWaterStopSensor),
  sensorMapping("hotWaterSensor", &hotWaterSensor),
  sensorMapping("pidSensor", &pidSensor),
  sensorMapping("compressorColdLimitSensor", &compressorColdLimitSensor),
  sensorMapping("compressorHotLimitSensor", &compressorHotLimitSensor),
  sensorMapping("bedroomTempSensor",&bedroomTempSensor)
};

bool SensorMapSetAddress(String sensorName, String address) {

  for (int i = 0; i < ARR_SENSORS; i++ )
  {
    sensorMapping *mapping = &SensorMap[i];
    if (sensorName == mapping->SensorName ) {
      DBG_OUTPUT_PORT.println("Set address for " + sensorName + " to " + address);
      if (address.length() == 16)
      {
        address.toUpperCase();
        byte addr[8];
        getAddressBytes(address, addr);
        copyByteArray(addr, mapping->sensorAddr, 8);
        mqtt.publish(String(MQTT_THERMOSTAT_PREFIX) + sensorName + String(MQTT_THERMOSTAT_ADDR_SUFFIX) + String("/status"), address);
        return true;

      } else
      {
        DBG_OUTPUT_PORT.println(F("ignoring bogus sensor address"));
        return false;
      }
    }
  }
  return false;
}
updateSensorToStatic(byte address[8], float curTemp) {
  for (int i = 0; i < ARR_SENSORS; i++ )
  {
    sensorMapping *mapping = &SensorMap[i];
    if (ByteArrayCompare(address, mapping->sensorAddr, 8))
    { 
      *mapping->Temperature = curTemp;
      mqtt.publish(String(MQTT_COMPRESSOR_TEMP_PREFIX)+ mapping->SensorName , String(curTemp));
      
    }
  }
}
