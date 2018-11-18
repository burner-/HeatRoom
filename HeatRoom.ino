#define HOSTNAME "tt29controller1"
#include <Controllino.h>
#define DBG_OUTPUT_PORT Serial
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include "mqttConf.h"
#include <PID_v1.h>
#include <avr/wdt.h>
#include <LinkedList.h>
#include <EEPROM.h>

/*
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb.h>
#include "communication.h"
*/

#include "global.h"
#include "config.h"
#include "sensormapping.h"
#include "limitmapping.h"
#include "scada.h"
#include "compressor.h"
#include "mqtt.h"





byte mac[] = {0xDE, 0xAD, 0xBE, 0x5F, 0x5B, 0x1E};

void setup()
{
  DBG_OUTPUT_PORT.begin(115200);
//  ReadConfig();
  IPAddress ip(10, 220, 2, 7);
  IPAddress myDns(8, 8, 8, 8);
  IPAddress gateway(10, 220, 0, 1);
  IPAddress subnet(255, 255, 0, 0);
  Ethernet.begin(mac, ip, myDns, gateway, subnet);

  Serial.println(F("Starting..."));
  Serial.println(Ethernet.localIP());
  
  initMQTT();
  initScada();
  initCompressorLogic();
  wdt_enable(WDTO_4S);
  
  Serial.println(String(limitMapGetTemp("compressorColdLimit")));
}


void loop() {
  wdt_reset();
  handleMQTT();
  handleScada();
  doCompressorLogic();
}
