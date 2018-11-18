#include <MQTT.h>
#include <MQTTClient.h>



//MQTT topics

#define MQTT_PREFIX "tt29/"
#define MQTT_START "configure/tt29/compressor"

//compressor.h
#define MQTT_COMPRESSOR_TEMP_PREFIX "tt29/compressor/temp/"
#define MQTT_THERMOSTAT_PREFIX "tt29/compressor/settings/" // prefix for all temp sensor address and setpoint set commands
#define MQTT_THERMOSTAT_ADDR_SUFFIX "/address" // 
#define MQTT_THERMOSTAT_SETPOINT_SUFFIX "/setpoint" // 

#define MQTT_COMPRESSOR_STATUS "tt29/compressor/status" // Compressor status
#define MQTT_COMPRESSOR_STATUS_PREFIX "tt29/compressor/state/"

#define MQTT_COMPRESSOR_GROUND_STATUS "tt29/compressor/state/ground/status" // status of ground loop (sink/ource)
#define MQTT_LOG_PREFIX "tt29/log/" // prefix for debug messages

// scada.h
#define MQTT_1WIRE_TEMP "1wire/temp/"




EthernetClient ethNet;
MQTTClient mqtt;

void connectMQTT() {
  Serial.print("\nconnecting mqtt...");
  while (!mqtt.connect(HOSTNAME)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  mqtt.subscribe("tt29/#");
}
handleMQTT(){
    mqtt.loop();
  if (!mqtt.connected()) {
    connectMQTT();
  }
}

void logToMQTT(String topic, String message)
{
  mqtt.publish(String(MQTT_LOG_PREFIX) + topic, message);
}
