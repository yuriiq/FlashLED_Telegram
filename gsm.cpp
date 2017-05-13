#include "gsm.h"
#include "SDCardRW.h"

const int USSDTime_ = 1000;
const int OKTime_ = 100;
const int tryCount_ = 5;  
const char * gsmLog = "GSMLOG.txt";
const char * GSMRecFileName = "tell.wav";

GSM::GSM(int rx, int tx) : _port(rx, tx) { 

}

void GSM::begin(long speed) {
    _port.begin(speed);
    int tryCount = tryCount_;     
    while (!sendCommandAndWaitOK ("AT")) {
      --tryCount;
      if (tryCount < 0) {
        SDCardRW.logToSD ("GSM Error!", gsmLog) ;
        return;        
      }
    }
    sendCommandAndWaitOK ("AT+GSMBUSY=1");
}

String GSM::lastMsg() const {
    return _buf;
}

void GSM::call(const String & tell, int timeout) {
  SDCardRW.logToSD ("call "+ tell, gsmLog) ;
  SDCardRW.startRec(GSMRecFileName);
  sendCommand (String("ATD" +tell+";").c_str());
  delay(timeout);
  sendCommand ("ATH0");
  SDCardRW.stopRec();
  delay(OKTime_);
  readResp(OKTime_);
}

String GSM::info() {
  SDCardRW.logToSD ("info", gsmLog) ;
  sendCommand ("AT+COPS?");
  readResp(OKTime_);
  return _buf;
}

String GSM::balance () {
  SDCardRW.logToSD ("balance", gsmLog) ;
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
  SDCardRW.logToSD (String("com:") + command, gsmLog) ;
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
  
  SDCardRW.logToSD ("resp:", gsmLog) ;
  SDCardRW.logToSD (_buf, gsmLog) ;
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