#define READ_ERROR_VALUE  -85 //fake temp value
const char* host = "roomcontroller1";


struct strConfig {

  boolean AutoTurnOff = true;
  boolean AutoTurnOn = true;
  double temp_start_main = 0;
  double temp_stop_main = 0;
  double temp_start_backup = 0;
  double temp_stop_backup = 0;
  int servoMin;
  int servoMax;

  // Compressor temp settings
  float requestedHotWaterTemp = 0; // temp < requestedHotWaterTemp = use pid for hot water valve
  float heatingHysteresis = 0; //requestedHotWaterTemp + heatingHysteresis = return to normal mode
  
  float coolWaterStartTemp = 0; // temp > requestedCoolWaterTemp = start compressor
  float coolWaterStopTemp = 0;
  float coolWaterHysteresis = 0; //requestedCoolWaterTemp - coolingHysteresis = stop temp

  
  float warmWaterStartTemp = 0; //Temperature for compressor start because of warming need
  float warmWaterStopHysteresis = 0; // This value is used to autocalculate stop/start temp
  float warmWaterStopTemp = 0;
  
  float compressorColdLimit = 0; // Force compressor to stop under this temperature.
  float compressorHotLimit = 0; // Force compressor to stop over this temperature.
  float compressorWaitTime = 20; // Minumum time to restart compressor again

  float bedroomTemp = 22;

  // Compressor pid settings
  double pidSetpoint; // Pid for compressor over heat protection
  double pidP;
  double pidI;
  double pidD;

  
}   config;
