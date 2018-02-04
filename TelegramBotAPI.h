
#ifndef TelegramBotAPI_h
#define TelegramBotAPI_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>
#include <SD.h>

enum ActionType {
  typing,
  upload_photo, 
  record_video,
  upload_video, 
  record_audio,
  upload_audio, 
  upload_document,
  find_location
};

enum KeyboardType {
  inlineKeyboardFlag = 0x1, 
  resizeFlag = 0x2, 
  oneTimeFlag = 0x4,
  selectiveFlag = 0x8
};

struct TelegramMessage
{
  int backMessageId;
  int chatId ;
  String text;
  String from;
};

class TelegramBotAPI
{
  public:
    TelegramBotAPI (const String & token, Client & client);
    bool getUpdates();
    bool sendMessage (const char * parse_mode = NULL) const;
    bool sendAudio(const String & fileName, int fileSize) const ;
    bool sendPhoto(const String & fileName, int fileSize) const ;
    bool sendMessageWithKeyboard(const JsonArray & keyboardBuffer, KeyboardType keyboardType) const;
    bool sendChatAction(ActionType action) const;   
    TelegramMessage message;
    
  private:
    bool sendPostMessage(const JsonObject& payload) const;
    bool sendGetMessage(const String& getCommand) const ;
    void sendGetToTelegram  (const String & command) const;
    void sendPostToTelegram(const JsonObject& payload) const ;
    void sendMediaToTelegram (const String & command, const String & properyName, const String & fileName, const String & contentType, int fileSize) const;
    String readResponse (unsigned int limit) const ; 
    bool checkResponse(const String & response) const ;
    int getOffset(const String & response) const ;
    int getChatId(const String & response) const;
    int getMessageId(const String & response) const ;
    int getIntVal(const String & response, const String & findStr) const ;
    String convertToUnicode(const String & data) const ;
    String convertFromUnicode(const String & unicodeStr) const ;
    bool checkMessage() const;
    
    const String _token;
    Client & _client;
    mutable unsigned long _offset;
};

#endif
