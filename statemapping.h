// work mode states
bool state_water_heating = false; // hot water heating
bool state_room_heating = false; //room radioators need more heat
bool state_cooling = false; // compressor running
bool compressorRun = false;
bool panicColdStopActive = false;
bool panicHotStopActive = false;
bool maintainceStop = false;
bool heating_isStarting = false;
bool flowValve = false;
bool compressorWait = true; // default true for preventing compressor start when temp sensors are not ready

class stateMapping {
  public:
    stateMapping(String stateName, bool* stateVariable);
    bool* State;
    String StateName;
};

stateMapping::stateMapping(String stateName, bool* stateVariable) {
  StateName = stateName;
  State = stateVariable;
}

#define ARR_STATES 10
stateMapping StateMap[ARR_STATES] = {
  stateMapping("state_water_heating", &state_water_heating),
  stateMapping("state_room_heating", &state_room_heating),
  stateMapping("state_cooling", &state_cooling),
  stateMapping("compressorRun", &compressorRun),
  stateMapping("panicColdStopActive", &panicColdStopActive),
  stateMapping("panicHotStopActive", &panicHotStopActive),
  stateMapping("maintainceStop", &maintainceStop),
  stateMapping("heating_isStarting", &heating_isStarting),
  stateMapping("flowValve", &flowValve),
  stateMapping("compressorWait", &compressorWait)
};

bool setState(bool* stateVariable, bool newState) {
  if (*stateVariable == newState)
    return false; // If no change, just return
  *stateVariable = newState;
  for (int i = 0; i < ARR_STATES; i++)
  {
    stateMapping mapping = StateMap[i];
    if (stateVariable == mapping.State)
    {
      mqtt.publish(MQTT_COMPRESSOR_STATUS_PREFIX + mapping.StateName + String("/status"), String(*mapping.State),true,0);
      return true;
    }
  }
  return true;
}
