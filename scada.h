
#include <OneWire.h>
#include <ArduinoJson.h>


#define ONEWIRE_PIN1 CONTROLLINO_D9
#define ONEWIRE_PIN2 CONTROLLINO_D10
#define ONEWIRE_PIN3 CONTROLLINO_D11

//#define COMPRESSOR2_PIN CONTROLLINO_R2

#define GROUNDPUMP_PIN CONTROLLINO_R5
#define GROUNDTANK_PIN CONTROLLINO_D8
#define PIN_SHUNT_BEDROOM CONTROLLINO_D0


IPAddress mcast(239, 0, 0, 20); // Multicast address
IPAddress bcast(255, 255, 255, 255); // Multicast address

EthernetUDP pbufUdp; // OBSOLETE

boolean compressorPowerOn = false; // user toggle for compressor
boolean compressor2PowerOn = true;
boolean groundPumpPowerOn = false;
boolean groundTankOn = false;



OneWire ds1(ONEWIRE_PIN1);
OneWire ds2(ONEWIRE_PIN2);
OneWire ds3(ONEWIRE_PIN3);

class SensorInfo
{
public:
  byte SensorAddress[8] = {};
  float Temperature = 0;
  boolean online = false;
  boolean local = false;
  OneWire *bus;
};




struct PIDSettings {
  int id = 0;
  double pidSetpoint = 0;
  double P = 0;
  double I = 0;
  double D = 0;
  byte SensorAddress[8] = {0,0,0,0,0,0,0,0};
};


class PidController
{
public:
  PIDSettings pidSettings = PIDSettings();
  double pidOutput = 0;
  double pidInput = 0;
//  String Name = "";
  int Pin;
  PID Controller = PID(&pidInput, &pidOutput, &pidSettings.pidSetpoint, 0, 0, 0, DIRECT);
};

LinkedList<PidController*> Pids = LinkedList<PidController*>();

void loadPid(int id,int pin){

  pinMode(pin, OUTPUT); 
  
  PidController *pid = new PidController();
  EEPROM_readGeneric(500 + (id * sizeof(PIDSettings)), pid->pidSettings);
  pid->pidSettings.id = id; //Set id each time so if eeprom return corruped data it always make sense
  
  Pids.add(pid);
  pid->Pin = pin;
  pid->Controller.SetOutputLimits(0, 255);
  //turn the PID on
  pid->Controller.SetMode(AUTOMATIC);
}


LinkedList<SensorInfo*> sensors = LinkedList<SensorInfo*>();


//Update temp to static variables and pids
void updateTemp(byte address[8], float curTemp)
{
  updateSensorToStatic(address,curTemp);
  
  for (int i = 0; i < Pids.size(); i++)
  {
    PidController *pid;  
    pid = Pids.get(i);
    
    if (ByteArrayCompare(pid->pidSettings.SensorAddress, address, 8))
    {
      pid->pidInput = curTemp;
    }
  }
  
}


void updateSensorInfo(OneWire *ds, byte address[8], float curTemp, boolean local)
{
  //update temp to static values
  updateTemp(address, curTemp);
  hexPrintArray(address,8);
  DBG_OUTPUT_PORT.print("\t");
  DBG_OUTPUT_PORT.println(curTemp);

  // publish generic mqtt topic
  String addrStr = "";
  getHexString(address, addrStr);
  mqtt.publish(String(MQTT_1WIRE_TEMP) + addrStr , String(curTemp));


  SensorInfo *sensor;
  for (int i = 0; i < sensors.size(); i++)
  {
    sensor = sensors.get(i);
    if (ByteArrayCompare(sensor->SensorAddress, address, 8))
    {
      sensor->online = true;
      sensor->local = local;
      sensor->Temperature = curTemp;
      sensor->bus = ds;
      return;
    }
  }

  SensorInfo *newsensor = new SensorInfo();
  copyByteArray(address, newsensor->SensorAddress, 8);
  newsensor->online = true;
  newsensor->local = local;
  newsensor->bus = ds;
  newsensor->Temperature = curTemp;
  sensors.add(newsensor);
  
  DBG_OUTPUT_PORT.println(F("Sensor added"));
}



void copyByteArrayToCharArray(byte in[], char out[],int len){
  for(int i=0;i<len;i++){
    out[i] = in[i];
  }
}


void doJob(){
  yield();
  handleMQTT();
}

void pollSleep(int time)
{
  time = time / 10;
  for (int i = 0; time > i; i++)
  {
    delay(10);
    doJob();
  }
}

void readTempSensor(OneWire *ds, byte addr[8], bool needConversion = false)
{
  int i;
  byte present = 0;
  byte data[9];
  float tempVal = 0;
  if (needConversion) // Each temp sensor need internal temp conversion from sensor to scratchpad
  {
    ds->reset();
    doJob();
    ds->select(addr);
    doJob();
    ds->write(0x44);
    pollSleep(750);
  }
  present = ds->reset();
  doJob();
  ds->select(addr); // select sensor
  doJob();
  ds->write(0xBE);         // Read Scratchpad
  doJob();
  
  // read measurement data
  for (i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds->read();
  }
  if (OneWire::crc8(data, 8) != data[8])
  {
    hexPrintArray(addr,8);
    DBG_OUTPUT_PORT.print(" \t");
    DBG_OUTPUT_PORT.println(F(" Data CRC is not valid!"));
    return;
  }
  int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
  // print also as human readable mode  
  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
  Whole = Tc_100 / 100;  // separate off the whole and fractional portions
  Fract = Tc_100 % 100;
  tempVal = (float)Whole;
  tempVal += ((float)Fract) / 100.0;
  if (SignBit) // If its negative
  {
    tempVal = 0 - tempVal;
  }
  doJob();
  if (tempVal != 85.00 && tempVal != -85)
  {
    updateSensorInfo(ds, addr, tempVal, true);
  } 
  else 
  {
    DBG_OUTPUT_PORT.print(F("Temp from localhost \t"));
    hexPrintArray(addr,8);
    DBG_OUTPUT_PORT.print(" \t");
    DBG_OUTPUT_PORT.println(F("Temp ERR -85"));
  }
}

// remove one offline sensor at each iteration
void cleanOfflineSensors(OneWire *ds)
{
  SensorInfo *sensor;
  for (int i = 0; i < sensors.size(); i++)
  {
    sensor = sensors.get(i);
    if (!sensor->online && sensor->local && sensor->bus == ds)
    {
      DBG_OUTPUT_PORT.println(F("Sensor removed"));
      sensors.remove(i);
      //clean more than just first
      cleanOfflineSensors(ds);
      return;
    }
  }
}

void markSensorsToOffline(OneWire *ds)
{
  SensorInfo *sensor;
  for (int i = 0; i < sensors.size(); i++)
  {
    sensor = sensors.get(i);
    if (sensor->local == true && sensor->bus == ds)
      sensor->online = false;
  }
}


void searchAllTempSensors(OneWire *ds)
{
  int count = 0;
  doJob();
  ds->reset_search();
  markSensorsToOffline(ds);
  byte addr[8];
  while (ds->search(addr)) {
    // debug print sensor address
    DBG_OUTPUT_PORT.print(F("Found "));
    hexPrintArray(addr,8);
    DBG_OUTPUT_PORT.println("");
    if (OneWire::crc8(addr, 7) == addr[7]) {
      if (addr[0] == 0x10) {
        //#DBG_OUTPUT_PORT.print("Device is a DS18S20 family device.\n");
        count++;
        readTempSensor(ds,addr, true);
      }
      else if (addr[0] == 0x28) {
        count++;
        //#DBG_OUTPUT_PORT.print("Device is a DS18B20 family device.\n");
        readTempSensor(ds,addr, true);
      }
      else {
        DBG_OUTPUT_PORT.print(F("Device family is not recognized: 0x"));
        DBG_OUTPUT_PORT.println(addr[0], HEX);
      }
    } 
    else 
    {
      DBG_OUTPUT_PORT.println(F(" Address CRC is not valid!"));
    }
  }
  // clean unfound sensors
  cleanOfflineSensors(ds);
  DBG_OUTPUT_PORT.print("All ");
  DBG_OUTPUT_PORT.print(count);
  DBG_OUTPUT_PORT.println(F(" sensors found"));
}

//#define 1WIRE_WAIT 1
//#define 1WIRE_CONTINUE 1

void prepareReadAllSensors(OneWire *ds)
{
    ds->reset();
    doJob();
    ds->skip();
    doJob();
    ds->write(0x44);  
}
void completeReadAllTempSensors(OneWire *ds)
{
    SensorInfo *sensor;
    for (int i = 0; i < sensors.size(); i++)
    {
      sensor = sensors.get(i);
      if(sensor->local && sensor->bus == ds)
        readTempSensor(ds, sensor->SensorAddress);
    }
    doJob();
}

void doFullTempSensorRead()
{
    prepareReadAllSensors(&ds1);
    prepareReadAllSensors(&ds2);
    prepareReadAllSensors(&ds3);
    pollSleep(750);
    completeReadAllTempSensors(&ds1);
    completeReadAllTempSensors(&ds2);
    completeReadAllTempSensors(&ds3);
}

void initScada()
{

  pinMode(GROUNDPUMP_PIN, OUTPUT); 
  pinMode(GROUNDTANK_PIN, OUTPUT); 

  pbufUdp.begin(8889);
  searchAllTempSensors(&ds1);
  searchAllTempSensors(&ds2);
  searchAllTempSensors(&ds3);
  
  //loadPid(0, PIN_SHUNT_OFFICE);
  //loadPid(1, PIN_SHUNT_VERANDA);


}
bool startByBackup = false;

unsigned long nexttempread = 0;
/*
void netRecvProtoBuffTempInfo(byte packetBuffer[], int packetSize )
{
  TempInfo message;
  pb_istream_t stream = pb_istream_from_buffer(packetBuffer, packetSize);
  bool status = pb_decode(&stream, TempInfo_fields, &message);
  if (!status)
  {
    DBG_OUTPUT_PORT.print(F("PB Message decoding failed: "));
    DBG_OUTPUT_PORT.println(PB_GET_ERROR(&stream));
  } 
  else
  {
     
    if (message.SensorAddr.size == 8)
    {
      updateSensorInfo(nullptr, message.SensorAddr.bytes,message.Temp, false);
    }
  }
}

bool readPbNetwork()
{
  //Process ethernet
  int packetSize =  pbufUdp.parsePacket();
  if(packetSize)
  {
    int messagetype =  pbufUdp.read();
    //DBG_OUTPUT_PORT.print(" messagetype: ");   
    //dumpType(messagetype);
    //DBG_OUTPUT_PORT.println();
    
    byte packetBuffer[UDP_TX_PACKET_MAX_SIZE - 1];
    pbufUdp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE - 1);
    
    yield();
  
    if (messagetype == 0x20)
    {
      netRecvProtoBuffTempInfo(packetBuffer, (packetSize -1));
    }
    return true;
  } 
  else 
    return false;


}
*/
void handleScada()
{
  
  if (nexttempread < millis())
  {
    doFullTempSensorRead();
    nexttempread = millis() + 5000;
  }
  
//  while (readPbNetwork()){}


  if (groundPumpPowerOn)
    digitalWrite(GROUNDPUMP_PIN, HIGH);
  else 
    digitalWrite(GROUNDPUMP_PIN, LOW);

  if (groundTankOn)
    digitalWrite(GROUNDTANK_PIN, HIGH);
  else 
    digitalWrite(GROUNDTANK_PIN, LOW);

  if (config.bedroomTemp > bedroomTempSensor){
    
  } else {
    
  }

  for (int i = 0; i < Pids.size(); i++)
  {
    PidController *pid;  
    pid = Pids.get(i);
    pid->Controller.Compute();
    analogWrite(pid->Pin, pid->pidOutput);
  }

}
