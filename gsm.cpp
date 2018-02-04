#include "gsm.h"
#include <Arduino.h>

#define DEBUGV(...) Serial.printf(__VA_ARGS__) 

const int USSDTime_ = 1000;
const int OKTime_ = 100;
const int tryCount_ = 5;  

GSM::GSM(int rx, int tx) : _port(rx, tx) { 

}

bool GSM::begin(long speed) {
  _port.begin(speed);
  int tryCount = tryCount_;     
  while (!sendCommandAndWaitOK ("AT")) {
    --tryCount;
    if (tryCount < 0) {
      DEBUGV("GSM Error!");
      return false;        
    }
  }
  hangup();
  sendCommandAndWaitOK ("AT+GSMBUSY=1");
  return true;
}


String GSM::updateMsg() {
  readResp(OKTime_);
  return _buf;
}

String GSM::lastMsg() const {
    return _buf;
}

String GSM::call(const String & tell, int timeout) {
  DEBUGV("call %s; ", tell.c_str()) ;
  sendCommandAndWaitOK (String("ATD" +tell+";").c_str()); 
  delay(timeout);
  readResp(OKTime_);
  String res = _buf;
  sendCommandAndWaitOK ("ATH0");
  return res;
}
void GSM::hangup() {
  sendCommandAndWaitOK ("ATH0");
}

String GSM::info() {
  DEBUGV("info\n") ;
  sendCommand ("AT+COPS?");
  readResp(OKTime_);
  return _buf;
}

String GSM::balance () {
  DEBUGV("balance\n") ;
  sendCommandAndWaitOK ("AT+CUSD=1,\"#100#\"");
  int tryCount = tryCount_;
  while (tryCount) {
    int i = _buf.indexOf("CUSD:");
    if (i > 0) {
      return getMessage('"', '"', i) ;
    }
    delay(USSDTime_);
    readResp(USSDTime_);
  }
  return _buf;
}

void GSM::sendCommand (const char * command) {
  DEBUGV("com: %s" ,command) ;
  _port.printf("%s\n", command);
}

void GSM::readResp(int time) {
  _buf = "";  
  const unsigned long sttime = millis();
  while (millis() < sttime + time) { 
    if (_port.available()) {
      const char a = _port.read();
      _buf += a;
    }
  }
  DEBUGV("resp: %s" ,_buf.c_str()) ;
}

bool GSM::sendCommandAndWaitOK(const char * command) {
  sendCommand (command);
  readResp(OKTime_);
  if (_buf.indexOf("OK") >= 0) return true;
  return false;
}

String GSM::getMessage( const char start, const char end, const int index) {
  const int i = _buf.indexOf(start, index)+1;
  const int j = _buf.indexOf(end, i);
  if (i <= 0 || j <= 0) return String();
  return _buf.substring(i,j);
}

