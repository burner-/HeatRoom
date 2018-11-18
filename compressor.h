#include <Servo.h> //Library manager
#include <Timer.h> //https://github.com/burner-/Timer.git

//pin definitions

#define PIN_FLOW_VALVE 1
#define PIN_COMPRESSOR_RUN CONTROLLINO_R2
#define PIN_MODE_VALVE_SERVO 2
#define MODBUS_COMPRESSOR_RUN 100

#define COMPRESSOR_PIN_STATE_STOP LOW
#define COMPRESSOR_PIN_STATE_RUN HIGH


// define persentage for each position
#define SERVO_WARM_WATER 100 // water for room heating
#define SERVO_HOT_WATER 0 // use water

// Ground sircullation mode
#define GROUND_SINK 1
#define GROUND_SOURCE 2
#include "statemapping.h"

Timer t;


Servo servo;
int groundmode = GROUND_SINK;


//compressor pid
double pidInput = 0;
double pidOutput = 0;
PID pid = PID(&pidInput, &pidOutput, &config.pidSetpoint, config.pidP, config.pidI, config.pidD, REVERSE);
bool autoconfigure = false;


void OpenFlowValve()
{
  //logMsg(6, F("Flow valve opened"));
  setState(&flowValve, true);
  digitalWrite(PIN_FLOW_VALVE, LOW);
}


void StartCompressor()
{
  if (!state_cooling && !state_room_heating) // start compressor only if it is still needed
  { 
    return;
  }
  //logMsg(6,"Compressor start");
  if ( panicHotStopActive || panicColdStopActive || compressorWait)
  {
    //SendSyslog(5,"ERROR: prevent compressor start at panic stop state.");
    //pollSleep(10000);
    t.after(10000, StartCompressor); // failed to start compressor so try again after some while
    return;
  }
  setState(&compressorRun, true);
  OpenFlowValve();
  digitalWrite(PIN_COMPRESSOR_RUN, COMPRESSOR_PIN_STATE_RUN);
}

void ServoDetach()
{
  //DBG_OUTPUT_PORT.println("Servo released");
  servo.detach();
}
int lastServoval = 0;
void SetServo(double val)
{
  char tmpString[10];  //  Hold The Convert Data
  dtostrf(val, 2, 2, tmpString);
  //logMsg(6,strcat("Servo turn ",tmpString));
  int servoval = (int) val;
  servoval = map(servoval, 0, 100, config.servoMin, config.servoMax);
  if (lastServoval != servoval) {
    servo.attach(PIN_MODE_VALVE_SERVO);
    servo.write(servoval);
    t.after(2000, ServoDetach);
    lastServoval = servoval;
  }
}


void StopWaterHeating()
{
  setState(&state_water_heating, false);
  SetServo(SERVO_WARM_WATER);
}

void CloseFlowValve()
{
  if (compressorRun == false)
  {
    //logMsg(6, F("Flow valve closed"));
    setState(&flowValve, false);
    digitalWrite(PIN_FLOW_VALVE, HIGH);
  }
  else
  {
    //logMsg(6,"Flow valve close ignored (compressor running)");
    setState(&flowValve, true);
    digitalWrite(PIN_FLOW_VALVE, LOW);
  }

}

void CompressorUnWait(){
      setState(&compressorWait, false);
}

void StopCompressor()
{
  StopWaterHeating(); // Start compressor next time in cooler exthaus enviroment
  if (setState(&compressorRun, false)) // if compressor state changes to stop (was not already stopped) then start close valve sequence and acticate compressor wait timer
  {
    t.after(10000, CloseFlowValve); // After 10 seconds close flow valve  
    setState(&compressorWait, true);
    int wait = int(config.compressorWaitTime) * 1000;
    logToMQTT("compressorWaitTime", "wait " + String(wait) + "ms");
    t.after(wait, CompressorUnWait); // After 10 seconds close flow valve
  }
  digitalWrite(PIN_COMPRESSOR_RUN, COMPRESSOR_PIN_STATE_STOP);
  
}

void setGroundMode(int mode)
{
  groundmode = mode;
}


void RequestCompressorStop()
{
  if (!state_room_heating && !state_cooling) // stop compressor if no need for warming or cooling
  {
    if (groundmode != GROUND_SINK)
      mqtt.publish(String(MQTT_COMPRESSOR_GROUND_STATUS), "SINK");
    setGroundMode(GROUND_SINK);
    StopCompressor();
  }
  else
  {
    if (state_room_heating)
    {
      //logMsg(6, F( "Stop Cooling continue room heating"));
      if (groundmode != GROUND_SOURCE)
        mqtt.publish(String(MQTT_COMPRESSOR_GROUND_STATUS), "SOURCE");
      setGroundMode(GROUND_SOURCE);
    }
    else if (state_cooling)
    {
      //logMsg(6, F("Stop room heating continue cooling"));
      if (groundmode != GROUND_SINK)
        mqtt.publish(String(MQTT_COMPRESSOR_GROUND_STATUS), "SINK");
      setGroundMode(GROUND_SINK);
    }
  }
}

void StartCooling()
{
  setState(&state_cooling, true);
  StartCompressor();
}

void StopCooling()
{
  setState(&state_cooling, false);
  RequestCompressorStop();
}

void StartWarming()
{
  setState(&state_room_heating, true);
  StartCompressor();
}



void StopWarming()
{
  setState(&state_room_heating, false);
  RequestCompressorStop();
}




void StartWaterHeating()
{
  setState(&state_water_heating, true); // set real status to true
  setState(&heating_isStarting, false); // set wait status to false
  SetServo(SERVO_HOT_WATER);

}


void PanicStop()
{
  //DBG_OUTPUT_PORT.println("PanicStop");
  StopWaterHeating();
  StopCooling();
  StopWarming();
  StopCompressor(); // this is really force shutdown for compressor
}

bool inPanic() {
  return (panicHotStopActive || panicColdStopActive);
}


void doCompressorLogic()
{
  t.update();
  
  if (maintainceStop)
  {
    if (state_cooling || state_room_heating)
    {
      //      logMsg(6, F("Moving to maintainceStop"));
      PanicStop();
    }
    return;
  }
  // Check panic values
  if (compressorHotLimitSensor > config.compressorHotLimit)
  {
    setState(&panicHotStopActive, true);
    PanicStop();
  }
  else
  {
    setState(&panicHotStopActive, false);
  }



  if (compressorColdLimitSensor < config.compressorColdLimit)
  {
    /*
        if (!panicColdStopActive)
        {
          mqtt.publish(String(MQTT_COMPRESSOR_COLD_PANIC_STATUS), "1");
          logToMQTT("panicColdStopActive", "compressorColdLimitSensor(" + String(compressorColdLimitSensor) + ") < config.compressorColdLimit(" + String(config.compressorColdLimit) + ")"  );
        }
        panicColdStopActive = true;
    */
    setState(&panicColdStopActive, true);
    PanicStop();
  }
  else
  {
    setState(&panicColdStopActive, false);
  }


  if (state_water_heating)
  {
    pidInput = pidSensor;
    pid.Compute();
    //        sendPidReport();
    SetServo(pidOutput);
    //logToMQTT("state_water_heating", "water_heating = true");
    if (config.requestedHotWaterTemp + config.heatingHysteresis < hotWaterSensor)
      StopWaterHeating();
  }
  else
  {
    //logToMQTT("state_water_heating", "water_heating = false");
    if (config.requestedHotWaterTemp > hotWaterSensor
        && state_cooling // open heating valve only if there is cooling need
        && !heating_isStarting // move to heating mode only once
       )
    {
      logToMQTT("state_water_heating", "Need for water heating");
      t.after(10000, StartWaterHeating); // after 10 secods turn to water heating mode
      setState(&heating_isStarting, true);
    }
    SetServo(SERVO_WARM_WATER); // Heating off
  }


  // Cooling
  if (!state_cooling)
  {
    if (config.coolWaterStartTemp < coolingStartSensor && !inPanic())
    {
      logToMQTT("state_cooling", "Cooling temperature warmer than requestedCoolWaterTemp");
      StartCooling();
    }
  }
  else if (state_cooling)
  {
    if (config.coolWaterStopTemp > coolingStopSensor && config.coolWaterStartTemp > coolingStartSensor)
    {
      logToMQTT("state_cooling", "Cooling temperature reached requestedCoolWaterTemp - coolingHysteresis ");
      StopCooling();
    }
  }

  // Room heating
  if (!state_room_heating)
  {
    if (config.warmWaterStartTemp > warmWaterStartSensor && !inPanic())
    {
      logToMQTT("state_room_heating", "Warming temp lower than warmWaterStartTemp");
      StartWarming();
    }
  }
  else if (state_room_heating)
  {
    if (config.warmWaterStopTemp < warmWaterStopSensor && config.warmWaterStartTemp < warmWaterStartSensor)
    {
      logToMQTT("state_room_heating", "Warming temp higher than warmWaterStopTemp");
      StopWarming();
    }
  }
}

void initCompressorLogic() {
  pinMode(PIN_COMPRESSOR_RUN, OUTPUT);
  pinMode(PIN_FLOW_VALVE, OUTPUT);
  digitalWrite(PIN_FLOW_VALVE, HIGH);
  digitalWrite(PIN_COMPRESSOR_RUN, COMPRESSOR_PIN_STATE_STOP);
  t.after(10000, CompressorUnWait); // prevent compressor start before temperature sensors are online

}
