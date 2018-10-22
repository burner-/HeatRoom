#include <Syslog.h> //https://github.com/burner-/Syslog
#include <utility/w5100.h>
void SendSyslog(uint8_t loglevel, const char message[])
{
 Syslog.logger(1,loglevel,HOSTNAME, message);
}
void SendSyslog(uint8_t loglevel, const __FlashStringHelper *ifsh)
{
  Syslog.logger(1,loglevel,HOSTNAME, ifsh);
}

void logMsg(int level, const __FlashStringHelper *ifsh)
{
          DBG_OUTPUT_PORT.println(ifsh);
         SendSyslog(level,ifsh);
}

void logMsg(int level, char msg[])
{
          DBG_OUTPUT_PORT.println(msg);
          SendSyslog(level,msg);
}
