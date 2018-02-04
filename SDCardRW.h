#ifndef SDCardRW_h
#define SDCardRW_h
#include <Arduino.h>

struct Settings {
    String SSID;   
    String PASS;  
    String toString() {
        return String("SSID:") + SSID + String("\nPASS:") + PASS + '\n';
    }
    void parse(char c) ;
private:
    String buf;
    enum {NONE, isSSID, isPASS } status = NONE;
};

class SDCardRWClass
{
public:
  SDCardRWClass() ;
  Settings getSettings(const String & fileName);
  bool startRec(const String & fileName);
  void stopRec();
  int samples();
  void setupSD();
  String infoFile(const String & fileName);
  int recSize (const String & fileName);
};

extern SDCardRWClass SDCardRW;


#endif
