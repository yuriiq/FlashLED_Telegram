#ifndef GSM_H
#define GSM_H

#include <SoftwareSerial.h>

extern const char * GSMRecFileName;

class GSM
{
public:
    GSM(int rx, int tx);
    bool begin(long speed);
    String call(const String & tell, int timeout);
    String balance();
    String info();
    String lastMsg() const;
    void hangup();
    String updateMsg() ;
        
private:
  void sendCommand (const char * command) ;
  bool sendCommandAndWaitOK(const char * command);
  void readResp(int time) ;
  String getMessage( const char start, const char end, const int index) ;
  SoftwareSerial _port; // (D8, D9);
  String _buf;
};

#endif
