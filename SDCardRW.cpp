
#include "SDCardRW.h"
#include <SD.h>
#include <time.h>

#define DEBUGV(...) Serial.printf(__VA_ARGS__) 

struct WavHeader
{    
    const char chunkId[4] = { 'R', 'I', 'F', 'F' };
    unsigned int chunkSize;
    const char format[4] = {'W', 'A', 'V', 'E'};
    const char subchunk1Id[4] = {'f', 'm', 't' , ' '};
    const unsigned int subchunk1Size = 16;
    const unsigned short audioFormat = 1;
    const unsigned short numChannels = 1;
    const unsigned int sampleRate = UPDATE_RATE;
    const unsigned int byteRate = UPDATE_RATE; // _header.byteRate = _header.sampleRate * _header.numChannels * _header.bitsPerSample/8;
    const unsigned short blockAlign = 1;
    const unsigned short bitsPerSample = 8;
    const char subchunk2Id[4] = { 'd', 'a', 't', 'a'} ;
    unsigned int subchunk2Size ;
};

#define SDFileLog "SDLog.txt" 
int _numSamples = -1; 
const int _recBufSize = 512;
const int sampleTime = ESP.getCpuFreqMHz() * 1000000 / UPDATE_RATE;
uint8_t _recBuf[_recBufSize];
unsigned int _now = 0;
File _recFile;

void corectFileName(String & fileName); 
void timerHandler ();

void Settings::parse(char c) {
    if (c == '\n') {
        switch (status) {
            case isSSID :
                SSID = buf;
                break;
            case isPASS :
                PASS = buf;
                break;
            case NONE:
                break;
        }
        buf = "";
        buf.reserve(1);
        status = NONE;
    } else {
        buf += c;
        if (buf == "SSID:") {
            buf = "";
            status = isSSID;
        } else if (buf == "PASS:") {
            buf = "";
            status = isPASS;
        }
    }
}

SDCardRWClass::SDCardRWClass() { 
    for (int i=0; i< _recBufSize; ++i) {
        _recBuf [i] = 127;
    }
}

void SDCardRWClass::setupSD() {
  if (SD.begin(D10)) {
    DEBUGV("card initialized.\n");
  } else {
    DEBUGV("Card failed, or not present\n");
  }
}

int SDCardRWClass::samples() { 
  return _numSamples; 
}

void SDCardRWClass::logToSD ( const String & msg, String fileName) {
  corectFileName(fileName);
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
      dataFile.println(String(millis()) +": "+ msg + "\n");
      dataFile.close();
  }
  DEBUGV("%s\n" , msg.c_str()) ;
}

Settings SDCardRWClass::getSettings(const String & fileName)
{
  logToSD("Get settings from " + fileName , SDFileLog);
  Settings settings;
  File dataFile = SD.open(fileName, FILE_READ);
  if (dataFile) {
      String buff;
      while (dataFile.available() > 0) {
          settings.parse(dataFile.read());
      }
      dataFile.close();
  } else {
      logToSD("Error to load " +fileName + ". Create default." , SDFileLog);
      dataFile = SD.open(fileName, FILE_WRITE);
      dataFile.println(settings.toString());
      dataFile.close();
  }
  logToSD(String("Settings in ") + fileName +"\n"+ settings.toString(), SDFileLog);
  return settings;
}

void SDCardRWClass::stopRec() {
  if (_numSamples > 0)  {
    timer0_detachInterrupt();
    WavHeader header;
    header.subchunk2Size = _numSamples * header.numChannels * header.bitsPerSample/8 + 512 - sizeof(WavHeader);
    header.chunkSize = header.subchunk2Size + 36; 
    int bufPos = _numSamples % _recBufSize;
    logToSD(String("StopRec at ") + _numSamples + "; " + bufPos, SDFileLog);
    _recFile.write(_recBuf, bufPos);
    _recFile.flush();
    _recFile.seek(0);
    _recFile.write ((const char *) & header, sizeof(WavHeader));
    _recFile.close();
    _numSamples = -1;
    // infoFile(_recFile.name());
  }
}

void SDCardRWClass::startRec(const String & fileName) {
  _recFile.close();
  _numSamples = 512 - sizeof(WavHeader); 
  String recFileName = fileName;
  corectFileName(recFileName);
  _recFile = SD.open(recFileName, FILE_WRITE);
  if (!_recFile)
  {
    logToSD("Failed open file" + recFileName, SDFileLog);
    return;
  }  
  _recFile.seek(0);
  _recFile.write(_recBuf, _recBufSize);
  // start Interrupts
  logToSD(String(sizeof(WavHeader)) + String(": start record to ") + _recFile.name() , SDFileLog);
  _now = millis();

  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(timerHandler);
  timer0_write(ESP.getCycleCount() + sampleTime);
  interrupts();
}

void timerHandler () {
  timer0_write(ESP.getCycleCount() -1);
  if (_numSamples > 0) 
  {
    unsigned int now  = ESP.getCycleCount();
    int a = analogRead(A0);
    a >>= 2;
    const int recBufPos = _numSamples % _recBufSize;
    _recBuf[recBufPos] = a;
    if (recBufPos >= _recBufSize - 1) {
      _recFile.write(_recBuf, _recBufSize);
    }
    ++ _numSamples;
    DEBUGV(": %d\n" , ESP.getCycleCount() - now) ;
    timer0_write(ESP.getCycleCount() + sampleTime);
  }
}

void corectFileName(String & fileName) {
  // fileName.remove(8, fileName.length() - 3);
  fileName.toUpperCase();
  fileName.replace('+', '_');
  fileName.replace(';', '_');
  fileName.replace('=', '_');
  fileName.replace('[', '_');
  fileName.replace(']', '_');
  fileName.replace('*', '_');
  fileName.replace('?', '_');
  fileName.replace('<', '_');
  fileName.replace(':', '_');
  fileName.replace('>', '_');
  fileName.replace('/', '_');
  fileName.replace('\\', '_');
  fileName.replace('|', '_');
  fileName.replace('"', '_');
}

String SDCardRWClass::infoFile(const String & fileName)
{
  File file = SD.open(fileName, FILE_READ);
  if (!file)
  {
      return "Failed open file";
  }
  WavHeader header;
  file.read (&header, sizeof(WavHeader));
  file.close();
  // Посчитаем длительность воспроизведения в секундах
  double fDurationSeconds = 1.f * header.subchunk2Size / (header.bitsPerSample / 8) / header.numChannels / header.sampleRate;
  int iDurationMinutes = (int)fDurationSeconds / 60;
  int iDurationSeconds = (int)fDurationSeconds % 60;
  // Выводим полученные данные
  String res = "chunkId: " + String(header.chunkId ).substring(0,3)
  + "\n chunkSize: "       + String(header.chunkSize)
  + "\n format: "          + String(header.format).substring(0,3)
  + "\n subchunk1Id: "     + String(header.subchunk1Id).substring(0,3)
  + "\n subchunk1Size: "   + String(header.subchunk1Size)
  + "\n audioFormat: "     + String(header.audioFormat)
  + "\n Channels: "        + String(header.numChannels)
  + "\n Sample rate: "     + String(header.sampleRate)
  + "\n byteRate: "        + String(header.byteRate)
  + "\n blockAlign: "      + String(header.blockAlign)
  + "\n Bits per sample: " + String(header.bitsPerSample)
  + "\n subchunk2Id: "     + String(header.subchunk2Id).substring(0,3)
  + "\n subchunk2Size: "   + String(header.subchunk2Size)
  + "\n Duration: "        + String(iDurationMinutes) +":"+ String(iDurationSeconds);
  return res;
}

SDCardRWClass SDCardRW;
