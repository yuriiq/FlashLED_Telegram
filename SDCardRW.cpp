
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
void infoFile(const String & fileName);

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
}

void SDCardRWClass::loopRec () {
  if (_numSamples >= 0 && millis() >= _now + sampleTime && _recFile) {
    _now = millis();
    int a = analogRead(A0);
    a >>= 2;
    const int recBufPos = _numSamples % _recBufSize;
    _recBuf[recBufPos] = a;
    if (recBufPos >= _recBufSize - 1) {
      _recFile.write(_recBuf, _recBufSize);
    }
    ++ _numSamples;
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

void SDCardRWClass::infoFile(const String & fileName)
{
    File file = SD.open(fileName, FILE_READ);
    if (!file)
    {
        printf("Failed open file\n");
        return ;
    }
    WavHeader header;
    file.read (&header, sizeof(WavHeader));
    // Выводим полученные данные

    Serial.printf("chunkId: %c%c%c%c\n", header.chunkId[0], header.chunkId[1], header.chunkId[2], header.chunkId[3]);
    Serial.printf("chunkSize: %d\n", header.chunkSize);      
    Serial.printf("format: %c%c%c%c\n", header.format[0], header.format[1], header.format[2], header.format[3]);
    Serial.printf("subchunk1Id: %c%c%c%c\n", header.subchunk1Id[0], header.subchunk1Id[1], header.subchunk1Id[2], header.subchunk1Id[3]);
    Serial.printf("subchunk1Size: %d\n", header.subchunk1Size);  
    Serial.printf("audioFormat: %d\n", header.audioFormat);    
    Serial.printf("Channels: %d\n", header.numChannels);
    Serial.printf("Sample rate: %d\n", header.sampleRate);
    Serial.printf("byteRate: %d\n", header.byteRate);    
    Serial.printf("blockAlign: %d\n", header.blockAlign);
    Serial.printf("Bits per sample: %d\n", header.bitsPerSample);
    Serial.printf("subchunk2Id: %c%c%c%c\n", header.subchunk2Id[0], header.subchunk2Id[1], header.subchunk2Id[2], header.subchunk2Id[3]);
    Serial.printf("subchunk2Size: %d\n", header.subchunk2Size);

    // Посчитаем длительность воспроизведения в секундах
    double fDurationSeconds = 1.f * header.subchunk2Size / (header.bitsPerSample / 8) / header.numChannels / header.sampleRate;
    int iDurationMinutes = floor(fDurationSeconds) / 60;
    int iDurationSeconds = fDurationSeconds - (iDurationMinutes * 60);
    Serial.printf("Duration: %02d:%02d \n", iDurationMinutes, iDurationSeconds);
    file.close();
}

SDCardRWClass SDCardRW;
