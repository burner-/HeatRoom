
//#include <SD.h>
#define HOSTNAME "tt29controller"

#include <ArduinoSTL.h>
#include <Controllino.h>

#define DBG_OUTPUT_PORT Serial
#define DEBUG_ESP_HTTP_SERVER
#define DEBUG_ESP_PORT Serial

#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#endif

//   #include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <EthernetWebServer.h>
#include <ModbusIP.h>
#include <PID_v1.h>
#include <avr/wdt.h>
#include "syslog.h"
#include <LinkedList.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "global.h"
#include "config.h"
//#include "wifi.h"

#include <pb_encode.h>
#include <pb_decode.h>
#include <pb.h>
#include "communication.h"
#include "scada.h"

#include "compressor.h"

#include "webserver.h"

boolean AdminEnabled = true;    // Enable Admin Mode for a given Time

// sensor book keeping
bool allFound = false;
int sensorCount = 0;




void handle_gauge()
{
  server.sendHeader("Access-Control-Allow-Origin", ORIGIN);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["main_sensor"] = temp_main;
  root["backup_sensor"] = temp_backup;

  char cbuf[256];
  root.prettyPrintTo(cbuf, sizeof(cbuf));
  server.send(200, "text/plain", cbuf);
}

void handle_sensors()
{

  int exceptedSize = (sensors.size() * 48) + 2;
  int sentSize = 0;
  server.setContentLength(exceptedSize);
  server.send(200, "text/plain", "[");

  SensorInfo *sensor;
  String retbuf = "";
  for (int i = 0; i < sensors.size(); i++)
  {
    retbuf = "";
    sensor = sensors.get(i);
    String sensorAddr;
    getHexString(sensor->SensorAddress, sensorAddr);
    retbuf += "{\"address\": \"";
    retbuf += sensorAddr;
    retbuf += "\", \"temp\": ";
    retbuf += sensor->Temperature;
    retbuf += "}";
    if (i + 1 < sensors.size())
      retbuf += ",\n";
    server.sendContent(retbuf);
    sentSize += retbuf.length();
  }

  retbuf = "]";
  sentSize++;
  while (exceptedSize > sentSize) {
    retbuf += "\n";
    sentSize++;
    if (retbuf.length() > 50)
    {
      server.sendContent(retbuf);
      retbuf = "";
    }
  }
  server.sendContent(retbuf);

}

void flushTempSensors()
{
  sensors.clear();
  searchAllTempSensors(&ds1);
  searchAllTempSensors(&ds2);
  searchAllTempSensors(&ds3);
  handle_sensors();
}

void handle_prometheus()
{
  int exceptedSize = (sensors.size() * 45) + 220;
  int sentSize = 0;
  server.setContentLength(exceptedSize);
  server.send(200, "text/plain", "");

  SensorInfo *sensor;
  String retbuf = "";
  for (int i = 0; i < sensors.size(); i++)
  {
    sensor = sensors.get(i);
    String sensorAddr;
    getHexString(sensor->SensorAddress, sensorAddr);
    retbuf = "";
    retbuf += "tempsensor{address=\"";
    retbuf += sensorAddr;
    retbuf += "\"} ";

    retbuf += sensor->Temperature;
    retbuf += "\n";
    server.sendContent(retbuf);
    sentSize += retbuf.length();
  }

  retbuf = "";
  retbuf += "compressor{id=\"3\"} ";
  if (compressorPowerOn)
    retbuf += "1\n";
  else
    retbuf += "0\n";

  retbuf += "compressor2PowerOn{id=\"2\"} ";
  if (compressor2PowerOn)
    retbuf += "1\n";
  else
    retbuf += "0\n";


  retbuf += "groundloop{id=\"1\"} ";
  if (groundPumpPowerOn)
    retbuf += "1\n";
  else
    retbuf += "0\n";

  sentSize += retbuf.length();
  server.sendContent(retbuf);

  // Compressor 2
  int idx = 1;
  retbuf = "";
  retbuf += "compressor{id=\"2\"} ";
  retbuf += Compressors[idx].compressorRun ? "1\n" : "0\n";

  retbuf += "water_heating{id=\"2\"} ";
  retbuf += Compressors[idx].state_water_heating ? "1\n" : "0\n";

  retbuf += "room_heating{id=\"2\"} ";
  retbuf += Compressors[idx].state_room_heating ? "1\n" : "0\n";

  sentSize += retbuf.length();
  server.sendContent(retbuf);
  retbuf = "";

  retbuf += "cooling{id=\"2\"} ";
  retbuf += Compressors[idx].state_cooling ? "1\n" : "0\n";

  retbuf += "panicColdStopActive{id=\"2\"} ";
  retbuf += Compressors[idx].panicColdStopActive ? "1\n" : "0\n";

  retbuf += "panicHotStopActive{id=\"2\"} ";
  retbuf += Compressors[idx].panicHotStopActive ? "1\n" : "0\n";

  retbuf += "maintainceStop{id=\"2\"} ";
  retbuf += Compressors[idx].maintainceStop ? "1\n" : "0\n";


  retbuf = "";
  while (exceptedSize > sentSize) {
    retbuf += "\n";
    sentSize++;
  }
  server.sendContent(retbuf);
  //server.send(200, "text/plain", "");
}



void handle_switches()
{
  server.sendHeader("Access-Control-Allow-Origin", ORIGIN);

  getArgumentValue("power", &compressorPowerOn);
  getArgumentValue("compressor2PowerOn", &compressor2PowerOn);

  getArgumentValue("groundPumpPowerOn", &groundPumpPowerOn);
  getArgumentValue("groundTankOn", &groundTankOn);
  if (server.hasArg("power") )
  {
    if (compressorPowerOn)
    {
      DBG_OUTPUT_PORT.println(F("Relay on by web"));
      startByBackup = false;
    }
    else if (!compressorPowerOn)
    {
      DBG_OUTPUT_PORT.println(F("Relay off by web"));
      startByBackup = false;
    }
  }

  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["power"] = boolToInt(compressorPowerOn);
  root["compressor2PowerOn"] = boolToInt(compressor2PowerOn);
  root["groundPumpPowerOn"] = boolToInt(groundPumpPowerOn);
  root["groundTankOn"] = boolToInt(groundTankOn);

  char cbuf[256];
  root.printTo(cbuf, sizeof(cbuf));
  server.send(200, "text/plain", cbuf);
}

int temperature;
int humidity;

void initWebserver()
{

  server.on("/switches", handle_switches);

  server.on("/general.html", recv_general_html);
  server.on("/pid/*", web_handle_pid);

  //server.on("/config.html", send_network_configuration_html);
  //server.on("/admin/values", send_network_configuration_values_html);
  //server.on("/admin/connectionstate", send_connection_state_values_html);
  server.on("/admin/generalvalues", send_general_configuration_values_html);

  server.on("/settings", set_settings);
  server.on("/gauge", handle_gauge);

  server.on("/metrics", handle_prometheus);
  server.on("/tempsensors", handle_sensors);
  server.on("/tempsensors/flush", flushTempSensors);

  //server.onNotFound(handle_unknown);
  server.begin();
  Serial.print(F("server is at "));
  Serial.println(Ethernet.localIP());

  temperature = 24;
  humidity = 40;

}

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0x5F, 0x6A, 0x1D
};
void setup()
{
  DBG_OUTPUT_PORT.begin(115200);
  ReadConfig();
  IPAddress ip(10, 220, 2, 7);
  IPAddress myDns(8, 8, 8, 8);
  IPAddress gateway(10, 220, 0, 1);
  IPAddress subnet(255, 255, 0, 0);
  modbus.config(mac, ip, myDns, gateway, subnet);

  Serial.println(F("Starting..."));
  Serial.println(Ethernet.localIP());

  initWebserver();
  initScada();
  wdt_enable(WDTO_4S);

}


void loop() {
  // handleOTA();
  wdt_reset();
  server.handleClient();
  yield();
  server.handleClient();
  yield();
  modbus.task();
  yield();
  //handleWebClients();
  handleScada();
}
