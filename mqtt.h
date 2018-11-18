
void publishCurrentConfig() {
  for (int i = 0; i < ARR_LIMITMAP; i++)
  {
    limitMapping mapping = TempMap[i];
    mqtt.publish(MQTT_THERMOSTAT_PREFIX + mapping.TopicName + String("/status"), String(*mapping.Temperature), true, 0);
  }
  for (int i = 0; i < ARR_SENSORS; i++ )
  {
    sensorMapping *mapping = &SensorMap[i];
    String addrStr = "";
    getHexString(mapping->sensorAddr, addrStr);
    mqtt.publish(String(MQTT_THERMOSTAT_PREFIX) + mapping->SensorName + String(MQTT_THERMOSTAT_ADDR_SUFFIX) + String("/status"), addrStr, true, 0);
  }
}

void mqttSetThermostatAddress(String &topic, String &payload) {
  String thermostatName = topic.substring(sizeof(MQTT_THERMOSTAT_PREFIX) - 1, topic.length() - sizeof(MQTT_THERMOSTAT_ADDR_SUFFIX) + 1);
  SensorMapSetAddress(thermostatName, payload);
}

void mqttSetThermostatSetpoint(String &topic, String &payload) {

  float tempval = payload.toFloat();
  String topicName = topic.substring(sizeof(MQTT_THERMOSTAT_PREFIX) - 1, topic.length() - sizeof(MQTT_THERMOSTAT_SETPOINT_SUFFIX) + 1);
  limitMapSetTemp(topicName, tempval);
}


// Handle received mqtt packet
void mqttReceived(String &topic, String &payload) {
  Serial.println("MQTT: " + topic + " " + payload);
  if (topic.startsWith(MQTT_THERMOSTAT_PREFIX) && topic.endsWith(MQTT_THERMOSTAT_ADDR_SUFFIX))
  {

    mqttSetThermostatAddress(topic, payload);
    //  saveConfig();
  }
  else if (topic.startsWith(MQTT_THERMOSTAT_PREFIX) && topic.endsWith(MQTT_THERMOSTAT_SETPOINT_SUFFIX))
  {
    mqttSetThermostatSetpoint(topic, payload);
    //  saveConfig();
  } else if (topic == String(MQTT_THERMOSTAT_PREFIX) + "get") {
    publishCurrentConfig();
  }
  else if (topic == String(MQTT_COMPRESSOR_STATUS_PREFIX) + "get") {
    for (int i = 0; i < ARR_STATES; i++)
    {
      stateMapping mapping = StateMap[i];
      mqtt.publish(MQTT_COMPRESSOR_STATUS_PREFIX + mapping.StateName + String("/status"), String(*mapping.State), true, 0);
    }
  }

}



void initMQTT() {
  mqtt.begin("37.16.111.26", ethNet);
  mqtt.onMessage(mqttReceived);
  connectMQTT();
  mqtt.publish(String(MQTT_START) , "requestconf");


}
