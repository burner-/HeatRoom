//#include <aREST.h>

#define ORIGIN "*"


//EthernetServer server(80);

void addStringValue(String &string,String valuename, String value)
{
  string += valuename + "|" + value +  "|input\n";
}

void addFloatValue(String &string,String valuename, float value)
{
  string += valuename + "|" + (String) value +  "|input\n";
}

void addIntValue(String &string,String valuename, int value)
{
  string += valuename + "|" + (String) value +  "|input\n";
}

int boolToInt(boolean bo)
{
  if (bo) return 1;
  else return 0;
}

void getArgumentValue(const char parmname[], boolean *target)
{
  
  if (server.hasArg(parmname))
  {
    
    String argVal = server.arg(parmname);
    if (argVal == "1" || argVal == "true")
    {
      DBG_OUTPUT_PORT.print(parmname);
      DBG_OUTPUT_PORT.println(" true");
      *target = true;
    }
    else 
    {
      DBG_OUTPUT_PORT.print(parmname);
      DBG_OUTPUT_PORT.println(" false");
      *target = false;
    }
  }
}
void getArgumentValue(const char parmname[], String *target)
{
  
  if (server.hasArg(parmname))
  {
    
    String argVal = server.arg(parmname);
    *target = urldecode(argVal);
  }
}


void getArgumentValue(const char parmname[], float *target)
{
  if (server.hasArg(parmname))
  {
    String argVal = server.arg(parmname);
    *target = argVal.toFloat();
  }
}
void getArgumentValue(const char parmname[], double *target)
{
  if (server.hasArg(parmname))
  {
    String argVal = server.arg(parmname);
    *target = argVal.toFloat();
  }
}

void getArgumentValue(const char parmname[], int *target)
{
  if (server.hasArg(parmname))
  {
    String argVal = server.arg(parmname);
    *target = argVal.toInt();
  }
}

bool setAddressConfigValue(const char parmname[], byte target[])
{
  if (server.hasArg(parmname))
  {
    String argVal = server.arg(parmname);
    byte addr[8];
    if (argVal.length() == 16)
    {
      argVal.toUpperCase();
      getAddressBytes(argVal, addr);
      copyByteArray(addr, target, 8);
      return true;
    }
    else
      DBG_OUTPUT_PORT.println(F("ignoring bogus sensor address"));
  }
  return false;
}



void send_general_configuration_values_html()
{
  server.sendHeader("Access-Control-Allow-Origin", ORIGIN);
  String values ="";
  values += "devicename|" +  (String)  stringConfig.DeviceName +  "|input\n";
  String sensorAddr;
  getHexString(config.temp_main_sensor, sensorAddr);
  addStringValue(values, "temp_main_sensor", sensorAddr);

  getHexString(config.temp_backup_sensor, sensorAddr);
  addStringValue(values, "temp_backup_sensor", sensorAddr);
   
  
  addFloatValue(values, "temp_start_main", config.temp_start_main);
  addFloatValue(values, "temp_stop_main", config.temp_stop_main);
  addFloatValue(values, "temp_start_backup", config.temp_start_backup);
  addFloatValue(values, "temp_stop_backup", config.temp_stop_backup);
  
  
  values += "toffenabled|" +  (String) (config.AutoTurnOff ? "checked" : "") + "|chk\n";
  values += "tonenabled|" +  (String) (config.AutoTurnOn ? "checked" : "") + "|chk\n";
  server.send ( 200, "text/plain", values);
  DBG_OUTPUT_PORT.println(__FUNCTION__); 
}

void web_handle_pid_setval(PidController *pid)
{
  bool changes= false;
  for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "setpoint")
      {
        pid->pidSettings.pidSetpoint = server.arg(i).toInt();
        changes=true;
      }
      if (server.argName(i) == "P")
      {
        pid->pidSettings.P = server.arg(i).toInt();
        changes=true;
      }
      if (server.argName(i) == "I")
      {
        pid->pidSettings.I = server.arg(i).toInt();
        changes=true;
      }
      if (server.argName(i) == "D")
      {
        pid->pidSettings.D = server.arg(i).toInt();
        changes=true;
      }
      if (server.argName(i) == "address")
      {
        changes = changes || setAddressConfigValue("address",pid->pidSettings.SensorAddress);
      }
  }
  
  pid->Controller.SetTunings(pid->pidSettings.P, pid->pidSettings.I, pid->pidSettings.D);
  if (changes)
    EEPROM_writeGeneric(500 + (pid->pidSettings.id * sizeof(PIDSettings)), pid->pidSettings);
  
  Serial.println("PID values set");
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  //root["name"] = pid->Name;
  String sensorAddr;
  getHexString(pid->pidSettings.SensorAddress, sensorAddr);
  root["address"] = sensorAddr;
  root["P"] = pid->pidSettings.P;
  root["I"] = pid->pidSettings.I;
  root["D"] = pid->pidSettings.D;
  root["setpoint"] = pid->pidSettings.pidSetpoint;
  root["input"] = pid->pidInput;
  root["output"] = pid->pidOutput;

  String buffer = "";
  char cbuf[150];

  root.printTo(cbuf, sizeof(cbuf));
  buffer = cbuf;
  server.send(200, "text/plain", buffer);
  
}
void web_handle_pid(){
  
  String uri = server.uri();
  uri.remove(0, 5);
  int id = uri.toInt();
  PidController *pid;
  for (int i = 0; i < Pids.size(); i++)
  {
    pid = Pids.get(i);
    if(pid->pidSettings.id == id)
    {
      web_handle_pid_setval(pid);
      return;
    }
  }
  
  
  server.send ( 404, "text/plain", "Pid not found");
}

void recv_general_html()
{
  server.sendHeader(F("Access-Control-Allow-Origin"), ORIGIN);
  if (server.args() > 0 )  // Save Settings
  {
    config.AutoTurnOn = false;
    config.AutoTurnOff = false;
    String temp = "";
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "devicename") stringConfig.DeviceName = urldecode(server.arg(i)); 
      if (server.argName(i) == "tonenabled") config.AutoTurnOn = true; 
      if (server.argName(i) == "toffenabled") config.AutoTurnOff = true; 
    }
   
    getArgumentValue("temp_start_main", &config.temp_start_main);
    getArgumentValue("temp_stop_main", &config.temp_stop_main);
    getArgumentValue("temp_start_backup", &config.temp_start_backup);
    getArgumentValue("temp_stop_backup", &config.temp_stop_backup);
    
    setAddressConfigValue("temp_main_sensor", config.temp_main_sensor);
    setAddressConfigValue("temp_backup_sensor", config.temp_backup_sensor);
    
    WriteConfig();
    
  }
  server.sendHeader("Cache-Control", F("no-transform,public,max-age=600,s-maxage=600"));
//  handleFileRead("/general.html");
  server.sendHeader("Location", String("http://scada.fsol.fi/general.html"), true);
  server.send ( 303, "text/html", "" ); 
  DBG_OUTPUT_PORT.println(__FUNCTION__); 
  
  
}

void jsonAddValue (String &string, String valuename, String value, bool newline)
{
  string += "\"" + valuename  + "\" : \"" + value + "\"" ;
  if (newline) string += ",\n";
}

void jsonAddValue (String &string, String valuename, float value, bool newline)
{
  string += "\"" + valuename  + "\" : \"" + (String) value + "\"" ;
  if (newline) string += ",\n";
}

void jsonAddValue (String &string, String valuename, double value, bool newline)
{
  string += "\"" + valuename  + "\" : \"" + (String) value + "\"" ;
  if (newline) string += ",\n";
}

void jsonAddValue (String &string, String valuename, int value, bool newline)
{
  string += "\"" + valuename  + "\" : \"" + (String) value + "\"" ;
  if (newline) string += ",\n";
}

void jsonAddValue (String &string, String valuename, boolean value, bool newline)
{
  string += "\"" + valuename  + "\" : ";
  if (value)
    string += "true"; 
  else 
    string += "false";
  if (newline) string += ",\n";
}

void set_settings()
{
  getArgumentValue("autoturnon", &config.AutoTurnOn);
  getArgumentValue("autoturnoff", &config.AutoTurnOff);
  getArgumentValue("pbsend", &config.pbSend);
  getArgumentValue("jsonsend", &config.jsonSend);
  
  getArgumentValue("devicename", &stringConfig.DeviceName); 
  
  getArgumentValue("temp_start_main", &config.temp_start_main);
  getArgumentValue("temp_stop_main", &config.temp_stop_main);
  getArgumentValue("temp_start_backup", &config.temp_start_backup);
  getArgumentValue("temp_stop_backup", &config.temp_stop_backup);
  setAddressConfigValue("temp_main_sensor", config.temp_main_sensor);
  setAddressConfigValue("temp_backup_sensor", config.temp_backup_sensor);

  
  for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "save") 
        WriteConfig();
  }
  String values = "{";
  
  
  jsonAddValue(values, "autoturnon", config.AutoTurnOn, true);
  jsonAddValue(values, "autoturnoff", config.AutoTurnOff, true);
  jsonAddValue(values, "pbsend", config.pbSend, true);
  jsonAddValue(values, "jsonsend", config.jsonSend, true);
  jsonAddValue(values, "devicename", stringConfig.DeviceName, true);   
  jsonAddValue(values, "temp_start_main", config.temp_start_main, true);
  jsonAddValue(values, "temp_stop_main", config.temp_stop_main, true);
  jsonAddValue(values, "temp_start_backup", config.temp_start_backup, true);
  jsonAddValue(values, "temp_stop_backup", config.temp_stop_backup, true);
  
  String sensorAddr;
  getHexString(config.temp_main_sensor, sensorAddr);
  jsonAddValue(values, "temp_main_sensor", sensorAddr, true);
  getHexString(config.temp_backup_sensor, sensorAddr);
  jsonAddValue(values, "temp_backup_sensor", sensorAddr, false);
  
  values += "\n}";
  server.send ( 200, "text/plain", values);
    
}









