/*
#include <Syslog.h>

void SendSyslog(uint8_t loglevel, char message[])
{
//  String num = String(settings.compressorId);
//  char b[2];
//  num.toCharArray(b,2);
  //Syslog.logger(1,5,strcat("Compressor",  b), message);
  Syslog.logger(1,loglevel,"compressor", message);
}
*/
