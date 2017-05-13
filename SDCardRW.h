#ifndef SDCardRW_h
#define SDCardRW_h

#define DEF_SSID "DIR-615"
#define DEF_PASS "80129820"

#include <Arduino.h>

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
  void logToSD (const String & msg, String fileName) ;
  Settings getSettings(const String & fileName);
  void startRec(const String & fileName);
  void stopRec();
  void loopRec();
  int samples();
  void setupSD();
  // void infoFile(const char * fileName);
};

extern SDCardRWClass SDCardRW;


#endif
