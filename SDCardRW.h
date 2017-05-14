#ifndef SDCardRW_h
#define SDCardRW_h
#include <Arduino.h>

#define DEF_SSID "DIR-615"
#define DEF_PASS "80129820"
#define UPDATE_RATE 8000 

struct Settings {
    String SSID = DEF_SSID;   
    String PASS = DEF_PASS;  
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
  void logToSD (const String & msg, String fileName) ;
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
