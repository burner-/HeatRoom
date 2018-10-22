#define READ_ERROR_VALUE  -85 //fake temp value
const char* host = "roomcontroller1";

struct stringConf {
  String ssid;
  String password;
  String ntpServerName;
  String DeviceName;
  
} stringConfig;

struct strConfig {
  char BOFmark = 'A';
  byte  IP[4] = {192,168,4,1};
  byte  Netmask[4] = {255,255,255,0};
  byte  Gateway[4] = {0,0,0,0};
  boolean dhcp = true;
  boolean AutoTurnOff = true;
  boolean AutoTurnOn = true;
  double temp_start_main = 0;
  double temp_stop_main = 0;
  double temp_start_backup = 0;
  double temp_stop_backup = 0;
  int servoMin;
  int servoMax;

  // Compressor temp settings
  double requestedHotWaterTemp; // temp < requestedHotWaterTemp = use pid for hot water valve
  double heatingHysteresis; //requestedHotWaterTemp + heatingHysteresis = return to normal mode
  double requestedCoolWaterTemp; // temp > requestedCoolWaterTemp = start compressor
  double coolingHysteresis; //requestedCoolWaterTemp - coolingHysteresis = stop temp
  double warmWaterStartTemp; //Temperature for compressor start because of warming need
  double warmWaterStopTemp; // Temperature for compressor stop because of enough warming heat
  double compressorColdLimit; // Force compressor to stop under this temperature.
  double compressorHotLimit; // Force compressor to stop over this temperature.

  // Reserve space in config struct for future extension
  double reserve1 = 0;
  double reserve2 = 0;
  double reserve3 = 0;
  double reserve4 = 0;
  double reserve5 = 0;
  double reserve6 = 0;
  double reserve7 = 0;
  double reserve8 = 0;
  boolean reserve9 =0;
  boolean reserve10 =0;
  boolean reserve11 =0;
  boolean reserve12 =0;
  boolean reserve13 =0;
  boolean reserve14 =0;

  // Compressor pid settings
  double pidSetpoint; // Pid for compressor over heat protection
  double pidP;
  double pidI;
  double pidD;

  
    // temp sensor onewire addresses
  byte temp_main_sensor[8] = {0,0,0,0,0,0,0,0}; //OneWire address for limit 1 sensor
  byte temp_backup_sensor[8] = {0,0,0,0,0,0,0,0}; //OneWire address for backup sensor
  byte pidSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for hot water shunt pid controlling
  byte hotWaterSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for hot water heating request
  byte coolingStartSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for cooling start
  byte coolingStopSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for cooling stop
  byte warmWaterStartSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for warming start
  byte warmWaterStopSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for warming stop
  byte compressorHotLimitSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for emergency stop
  byte compressorColdLimitSensorAddr[8] = {0,0,0,0,0,0,0,0}; //Sensor address for emergency stop

  // Reserve space in config struct for future extension
  byte reserve1SensorAddr[8] = {0,0,0,0,0,0,0,0};
  byte reserve2SensorAddr[8] = {0,0,0,0,0,0,0,0};
  byte reserve3SensorAddr[8] = {0,0,0,0,0,0,0,0};
  byte reserve4SensorAddr[8] = {0,0,0,0,0,0,0,0};
  byte reserve5SensorAddr[8] = {0,0,0,0,0,0,0,0};
  byte reserve6SensorAddr[8] = {0,0,0,0,0,0,0,0};
  byte reserve7SensorAddr[8] = {0,0,0,0,0,0,0,0};
  byte reserve8SensorAddr[8] = {0,0,0,0,0,0,0,0};

  
  
  boolean pbSend = true;
  boolean jsonSend = true;
  char EOFmark = 'E';
}   config;



boolean ReadStringConfig()
{
  DBG_OUTPUT_PORT.println(F("Reading string configuration"));
  if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F'  && EEPROM.read(2) == 'G' )
  {
    DBG_OUTPUT_PORT.println("Configuration Found");
    stringConfig.ssid = ReadStringFromEEPROM(32);
    stringConfig.password = ReadStringFromEEPROM(64);
    stringConfig.ntpServerName = ReadStringFromEEPROM(96);
    stringConfig.DeviceName= ReadStringFromEEPROM(128);
    return true;
  }
  else
  {
    DBG_OUTPUT_PORT.println(F("Configurarion not found!"));
    return false;
  }
}

boolean ReadConfig()
{
  int count = EEPROM_readGeneric(300, config);
  DBG_OUTPUT_PORT.print("Bytes read: ");
  DBG_OUTPUT_PORT.println(count);
  if (config.BOFmark == 'A' && config.EOFmark == 'E')
  {
    DBG_OUTPUT_PORT.println(F("Configuration found"));
    return ReadStringConfig();    
  }
  else 
  {
    DBG_OUTPUT_PORT.print(F("Cannot find configuration! BOF "));
    DBG_OUTPUT_PORT.print(config.BOFmark);
    DBG_OUTPUT_PORT.print(" EOF ");
    DBG_OUTPUT_PORT.println(config.EOFmark);
    return false;
  }
}

void WriteStringConfig()
{
  DBG_OUTPUT_PORT.println(F("Writing string Config"));
  EEPROM.write(0,'C');
  EEPROM.write(1,'F');
  EEPROM.write(2,'G');
  WriteStringToEEPROM(32,stringConfig.ssid);
  WriteStringToEEPROM(64,stringConfig.password);
  WriteStringToEEPROM(96,stringConfig.ntpServerName);
  WriteStringToEEPROM(128,stringConfig.DeviceName);
  //EEPROM.update();
}

void WriteConfig()
{
  
  DBG_OUTPUT_PORT.print(F("Writing config to memory "));
  config.BOFmark = 'A';
  config.EOFmark = 'E';
  int count = EEPROM_writeGeneric(300, config);
  DBG_OUTPUT_PORT.print(count);
  
  DBG_OUTPUT_PORT.println("Bytes writed");
  WriteStringConfig();
}



void resetconfig()
{
  DBG_OUTPUT_PORT.println(F("Restoring defaults"));
      // DEFAULT CONFIG
    stringConfig.ssid = "MYSSID";
    stringConfig.password = "";
    config.dhcp = true;
    config.IP[0] = 192; config.IP[1] = 168; config.IP[2] = 1; config.IP[3] = 100;
    config.Netmask[0] = 255; config.Netmask[1] = 255; config.Netmask[2] = 255; config.Netmask[3] = 0;
    config.Gateway[0] = 192; config.Gateway[1] = 168; config.Gateway[2] = 1; config.Gateway[3] = 1;
    stringConfig.DeviceName = "Not Named";
    config.AutoTurnOff = false;
    config.AutoTurnOn = false;
    config.temp_start_main = 45;
    config.temp_stop_main = 30;
    config.temp_start_backup = 25;
    config.temp_stop_backup = 10; 
  
  //  WriteConfig();
    DBG_OUTPUT_PORT.println(F("General config applied"));
 }

 
