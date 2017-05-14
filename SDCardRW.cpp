
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
const int sampleTime = ESP.getCpuFreqMHz() * 1000000 / UPDATE_RATE;
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

Settings SDCardRWClass::getSettings(const String & fileName)
{
  DEBUGV("Get settings from %s\n" , fileName.c_str());
  Settings settings;
  File dataFile = SD.open(fileName, FILE_READ);
  if (dataFile) {
      String buff;
      while (dataFile.available() > 0) {
          settings.parse(dataFile.read());
      }
      dataFile.close();
  } else {
      DEBUGV("Error to load %s, create default\n", fileName.c_str() );
      dataFile = SD.open(fileName, FILE_WRITE);
      dataFile.println(settings.toString());
      dataFile.close();
  }
  DEBUGV("Settings in %s\n%s\n", fileName.c_str() , settings.toString().c_str());
  return settings;
}
  
void SDCardRWClass::stopRec() {
  if (_numSamples > 0)  {
    timer0_detachInterrupt();
    WavHeader header;
    header.subchunk2Size = _numSamples * header.numChannels * header.bitsPerSample/8 + 512 - sizeof(WavHeader);
    header.chunkSize = header.subchunk2Size + 36; 
    // logToSD(String("StopRec at ") + _numSamples , SDFileLog);
    _recFile.seek(0);
    _recFile.write ((const char *) & header, sizeof(WavHeader));
    _recFile.close();
    _numSamples = -1;
    delay(10);
    DEBUGV("%s\n", infoFile(_recFile.name()).c_str());
  }
}

bool SDCardRWClass::startRec(const String & fileName) {
  _recFile.close();
  _numSamples = 0; 
  String recFileName = fileName;
  corectFileName(recFileName);
  _recFile = SD.open(recFileName, FILE_WRITE);
  if (!_recFile)
  {
    DEBUGV("Failed open file %d\n", recFileName.c_str());
    return false;
  }
  WavHeader header; 
  _recFile.seek(0);
  _recFile.write((uint8_t*)& header, sizeof(WavHeader));
  // start Interrupts
  DEBUGV(": start record to %s\n", _recFile.name());
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(timerHandler);
  timer0_write(ESP.getCycleCount() + sampleTime);
  interrupts();
  return true;
}

void timerHandler () {
  timer0_write(ESP.getCycleCount() -1);
  if (_numSamples >= 0) 
  {
    timer0_write(ESP.getCycleCount() + sampleTime);
    unsigned int a = analogRead(A0);
    _recFile.write((const unsigned char) a);
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

int SDCardRWClass::recSize (const String & fileName)
{
  File file = SD.open(fileName, FILE_READ);
  if (!file)
  {
      return -1;
  }
  WavHeader header;
  file.read (&header, sizeof(WavHeader));
  file.close();
  return header.chunkSize + 8; 
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
