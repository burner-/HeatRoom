#include <Servo.h> //Library manager
#include <Timer.h> //https://github.com/burner-/Timer.git



//pin definitions

#define PIN_FLOW_VALVE 1
#define PIN_COMPRESSOR_RUN 2
#define PIN_MODE_VALVE_SERVO 2


// define persentage for each position
#define SERVO_WARM_WATER 100 // water for room heating
#define SERVO_HOT_WATER 0 // use water

// Ground sircullation mode
#define GROUND_SINK 1
#define GROUND_SOURCE 2

Timer t;


Servo servo;
int groundmode = GROUND_SINK;

// work mode states
bool state_water_heating = false; // hot water heating
bool state_room_heating = false; //room radioators need more heat
bool state_cooling = false; // compressor running
bool compressorRun = false;
bool panicColdStopActive = false;
bool panicHotStopActive = false;
boolean maintainceStop = false;
bool heating_isStarting = false;




//compressor pid
double pidInput = 0;
double pidOutput = 0;
PID pid = PID(&pidInput, &pidOutput, &config.pidSetpoint, config.pidP, config.pidI, config.pidD, REVERSE);
bool autoconfigure = false;


void OpenFlowValve()
{
  logMsg(6, F("Flow valve opened"));
  digitalWrite(PIN_FLOW_VALVE, LOW);
}

void StartCompressor()
{
  //logMsg(6,"Compressor start");
  if ( panicHotStopActive || panicColdStopActive)
  {
    //    //SendSyslog(5,"ERROR: prevent compressor start at panic stop state.");
    pollSleep(10000);
    return;
  }
  compressorRun = true;
  OpenFlowValve();

  logMsg(5, F("Compressor started"));
  digitalWrite(PIN_COMPRESSOR_RUN, LOW);
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
  logMsg(6, F("Water heating stopped"));
  state_water_heating = false;
  SetServo(SERVO_WARM_WATER);
}

void CloseFlowValve()
{
  if (compressorRun == false)
  {
    logMsg(6, F("Flow valve closed"));
    digitalWrite(PIN_FLOW_VALVE, HIGH);
  }
  else
  {
    //logMsg(6,"Flow valve close ignored (compressor running)");
    digitalWrite(PIN_FLOW_VALVE, LOW);
  }

}

void StopCompressor()
{
  StopWaterHeating(); // Start compressor next time in cooler exthaus enviroment
  compressorRun = false;
  logMsg(6, F("Compressor stopped"));
  digitalWrite(PIN_COMPRESSOR_RUN, HIGH);
  t.after(10000, CloseFlowValve); // After 10 seconds close flow valve
}

void setGroundMode(int mode)
{
  groundmode = mode;
}


void RequestCompressorStop()
{
  if (!state_room_heating && !state_cooling) // stop compressor if no need for warming or cooling
  {
    setGroundMode(GROUND_SINK);
    StopCompressor();
  }
  else
  {
    if (state_room_heating)
    {
      logMsg(6, F( "Stop Cooling continue room heating"));
      setGroundMode(GROUND_SOURCE);
    }
    else if (state_cooling)
    {
      logMsg(6, F("Stop room heating continue cooling"));
      setGroundMode(GROUND_SINK);
    }
  }
}

void StartCooling()
{
  state_cooling = true;
  logMsg(6, F("Start Cooling"));
  StartCompressor();
}

void StopCooling()
{
  state_cooling = false;
  logMsg(6, F("Stop Cooling"));
  RequestCompressorStop();
}

void StartWarming()
{
  state_room_heating = true;
  logMsg(6, F("Start room heating"));
  StartCompressor();
}



void StopWarming()
{
  state_room_heating = false;
  logMsg(6, F("Stop room heating"));
  RequestCompressorStop();
}




void StartWaterHeating()
{
  state_water_heating = true;
  heating_isStarting = false;
  SetServo(SERVO_HOT_WATER);
  logMsg(6, F("Water heating started"));
}


void PanicStop()
{
  StopWaterHeating();
  StopCooling();
  StopCompressor(); // this is really force shutdown for compressor
  state_cooling = false;
  state_room_heating = false;
}

bool inPanic() {
  return (panicHotStopActive || panicColdStopActive);
}


void doLogic(byte addr[8], float tempVal)
{
  if (maintainceStop)
  {
    if (state_cooling || state_room_heating)
    {
      logMsg(6, F("Moving to maintainceStop"));
      PanicStop();
    }
    return;
  }
  // Check panic values
  compressorHotLimitSensor = tempVal;
  if (compressorHotLimitSensor > config.compressorHotLimit)
  {
    if (!panicHotStopActive)
      logMsg(6, F("Hot water over temp. Panic stop"));
    panicHotStopActive = true;
    PanicStop();
  }
  else
  {
    if (panicHotStopActive)
      logMsg(6, F("Hot water temp back in range. Panic cancelled"));
    panicHotStopActive = false;
  }



  if (compressorColdLimitSensor < config.compressorColdLimit)
  {
    if (!panicColdStopActive)
      logMsg(6, F("Cold water under temp. Panic stop"));
    panicColdStopActive = true;
    PanicStop();
  }
  else
  {
    if (panicColdStopActive)
      logMsg(6, F("Cold water temp back in range. Panic cancelled"));

    panicColdStopActive = false;
  }



  // FIXME merge with next block
  if (state_water_heating)
  {
    pidInput = pidSensor;
    pid.Compute();
    //        sendPidReport();
    SetServo(pidOutput);
  }
  else
  {
    SetServo(SERVO_WARM_WATER); // Heating off
  }


  if (state_water_heating)
  {
    //DBG_OUTPUT_PORT.println("water_heating = true");
    if (config.requestedHotWaterTemp + config.heatingHysteresis < hotWaterSensor)
      StopWaterHeating();
  }
  else
  {
    //DBG_OUTPUT_PORT.println("water_heating = false");
    if (config.requestedHotWaterTemp > hotWaterSensor
        && state_cooling // open heating valve only if there is cooling need
        && !heating_isStarting // move to heating mode only once
       )
    {
      logMsg(6, F("Need for water heating"));
      t.after(10000, StartWaterHeating); // after 10 secods turn to water heating mode
      heating_isStarting = true;
    }
  }


  // Cooling
  if (!state_cooling)
  {
    if (config.requestedCoolWaterTemp < coolingStartSensor && !inPanic())
    {
      logMsg(7, F("Cooling temperature warmer than requestedCoolWaterTemp"));
      StartCooling();
    }
  }
  else if (state_cooling)
  {
    if (config.requestedCoolWaterTemp - config.coolingHysteresis > coolingStopSensor)
    {
      logMsg(7, F("Cooling temperature reached requestedCoolWaterTemp - coolingHysteresis "));
      StopCooling();
    }
  }

  // Room heating
  else if (!state_room_heating)
  {
    if (config.warmWaterStartTemp > warmWaterStartSensor && !inPanic())
    {
      logMsg(7, F("Warming temp lower than warmWaterStartTemp"));
      StartWarming();
    }
  }
  else if (state_room_heating)
  {
    if (config.warmWaterStopTemp < warmWaterStopSensor)
    {
      logMsg(7, F("Warming temp higher than warmWaterStopTemp"));
      StopWarming();
    }
  }
}

void initLogic() {
  pinMode(PIN_COMPRESSOR_RUN, OUTPUT);
  pinMode(PIN_FLOW_VALVE, OUTPUT);
  digitalWrite(PIN_FLOW_VALVE, HIGH);
  digitalWrite(PIN_COMPRESSOR_RUN, HIGH);

}
