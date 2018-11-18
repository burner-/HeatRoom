class limitMapping {
public:
  limitMapping(String topicName, float* TempVariable);
  float* Temperature;
  String TopicName;
  void sync();
};

limitMapping::limitMapping(String topicName, float* TempVariable){
  Temperature = TempVariable;
  TopicName = topicName;
}

#define ARR_LIMITMAP 12
limitMapping TempMap[ARR_LIMITMAP] = {
  limitMapping("compressorColdLimit",&config.compressorColdLimit),
  limitMapping("compressorHotLimit",&config.compressorHotLimit),
  limitMapping("requestedHotWaterTemp",&config.requestedHotWaterTemp),
  limitMapping("heatingHysteresis",&config.heatingHysteresis),
  
  limitMapping("coolWaterStartTemp",&config.coolWaterStartTemp),
  limitMapping("coolWaterStopTemp",&config.coolWaterStopTemp),
  limitMapping("coolWaterHysteresis",&config.coolWaterHysteresis),
  
  limitMapping("warmWaterStartTemp",&config.warmWaterStartTemp),
  limitMapping("warmWaterStopTemp",&config.warmWaterStopTemp),
  limitMapping("warmWaterStopHysteresis",&config.warmWaterStopHysteresis),
  limitMapping("compressorWaitTime",&config.compressorWaitTime),
  limitMapping("bedroomTemp",&config.bedroomTemp)
};

float limitMapGetTemp(String topicName){
  for(int i = 0;i < ARR_LIMITMAP;i++ )
  {
    limitMapping mapping = TempMap[i];
    if (topicName == mapping.TopicName ){
      return *mapping.Temperature;
    }
  }
  return 0;
}


void limitMapSetTemp(String topicName, float temp); // generate prototype so that compiler does not barf

void sendSetPoint(String topicName, float temp){
  mqtt.publish(MQTT_THERMOSTAT_PREFIX + topicName + MQTT_THERMOSTAT_SETPOINT_SUFFIX, String(temp), true, 0);
  limitMapSetTemp(topicName, temp);
}

// Some types need sync
void handleSpecials(String topicName, float temp){
  if (topicName == "coolWaterStartTemp" || topicName == "coolWaterHysteresis") 
    sendSetPoint("coolWaterStopTemp", config.coolWaterStartTemp - config.coolWaterHysteresis);
  
}

void limitMapSetTemp(String topicName, float temp)
{
  for(int i = 0;i < ARR_LIMITMAP;i++ )
  {
    limitMapping mapping = TempMap[i];
    if (topicName == mapping.TopicName ){
      *mapping.Temperature = temp;
      //if (recursive) mapping.sync();
      mqtt.publish(MQTT_THERMOSTAT_PREFIX + topicName + MQTT_THERMOSTAT_SETPOINT_SUFFIX + String("/status"), String(temp), true, 0);
      handleSpecials(topicName,temp);
      return;
    }
  }
  DBG_OUTPUT_PORT.println("unknown limit name " + topicName);
  return;
}
