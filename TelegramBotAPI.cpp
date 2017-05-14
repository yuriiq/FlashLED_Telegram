
#include "TelegramBotAPI.h"

// #define DEBUGV(...) Serial.printf(__VA_ARGS__) 
// ets_printf(__VA_ARGS__)

#define host_ "api.telegram.org"
const byte tryCount_ = 5;
const unsigned short sslPort_ = 443;
const unsigned short bufferSize_ = 1024 ;
const unsigned short readDelay_ = 1800;
const unsigned short sendDelay_ = 5000;
const unsigned short maxMessageLength_ = 2000;
const unsigned short maxResponse_ = 500;

TelegramBotAPI::TelegramBotAPI (const String & token, Client & client)
  : _token (token), _client (client), _offset(0)  {}

bool TelegramBotAPI::getUpdates() {
  DEBUGV(":getUpdates:offset=%ld ", _offset);
  message.chatId = 0;
  message.backMessageId = 0;
  message.text = "";
  message.from = "";
  bool isMessage = false;
  const String command ( "bot" + _token + "/getUpdates?offset=" + String(_offset) + "&limit=1" );
  sendGetToTelegram(command); 
  String response = readResponse(maxMessageLength_); 
  if (response.length() > 0) {
    response = convertFromUnicode(response);
    DynamicJsonBuffer jsonBuffer;
    const JsonObject& root = jsonBuffer.parseObject(response);
    if (root.success() && root.containsKey("result")) {
        const unsigned int resultArrayLength = root["result"].size();
        if (resultArrayLength > 0 ) {
          const JsonObject& resultItem = root["result"][0];
          const unsigned int update_id = resultItem["update_id"];
          _offset = update_id + 1;
          if (resultItem.containsKey("callback_query")) {
            const JsonObject& jMessage = resultItem["callback_query"] ;
            message.chatId = jMessage["message"]["chat"]["id"];
            message.backMessageId = jMessage["message"]["message_id"]; 
            String data = jMessage["data"] ;
            message.text = data;
            String from = jMessage["from"]["first_name"];
            message.from = from;
            isMessage = true;
          } else if (resultItem.containsKey("message")) {
            const JsonObject& jMessage = resultItem["message"]; 
            message.chatId = jMessage["chat"]["id"];
            String text = jMessage["text"];
            message.text = text;
            String from = jMessage["from"]["first_name"];
            message.from = from;
            isMessage = true;
          } else {
            DEBUGV(" unknown response ");
          }
        } else {
          DEBUGV(" no messages ");
        }
    } else {
      DEBUGV("Parse response error.") ;
      isMessage = true;
      _offset = getOffset(response) + 1;
      message.chatId = getChatId(response);
    }
  } else {
    DEBUGV("Response is NULL.") ;
  }
  return isMessage;
}

bool TelegramBotAPI::sendMessage (const char * parse_mode) const {
  DEBUGV(":SendMessage ");
  if (message.chatId == 0) {
    DEBUGV("Error: chatId=0\n");
    return false;
  }
  if (!checkMessage()) {
    DEBUGV("Error in checkMessage\n");
    return false;
  }
  DynamicJsonBuffer jsonBuffer;
  JsonObject& payload = jsonBuffer.createObject();
  payload["chat_id"] = message.chatId;
  payload["text"] = message.text;
  if (parse_mode) {
    payload["parse_mode"] = parse_mode;
  }
  if (message.backMessageId) {
    payload["message_id"] = message.backMessageId;
  }
  return sendPostMessage(payload);
}

bool TelegramBotAPI::sendAudio(const String & fileName) const {
  sendChatAction(upload_audio);
  const int index = fileName.lastIndexOf('.') + 1;
  if (index <= 0) return false;
  const String contentType = "audio/" + fileName.substring(index);
  sendMediaToTelegram ("sendAudio", "audio", fileName, contentType);
  const String response = readResponse(maxResponse_);
  if (checkResponse(response)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool TelegramBotAPI::sendPhoto(const String & fileName) const {
  sendChatAction(upload_photo);
  const int index = fileName.lastIndexOf('.') + 1;
  if (index <= 0) return false;
  const String contentType = "image/" + fileName.substring(index);
  sendMediaToTelegram ("sendPhoto", "photo", fileName, contentType) ;
  const String response = readResponse(maxResponse_);
  if (checkResponse(response)) {
    return true;
  } else {
    return false;
  }
  return false;
}
bool TelegramBotAPI::sendMessageWithKeyboard (const JsonArray & keyboardBuffer, KeyboardType keyboardType) const {
  DEBUGV(":SendMessageWithKeyboard ");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& payload = jsonBuffer.createObject();
  payload["chat_id"] = message.chatId;
  payload["text"] = message.text;
  if (message.backMessageId && (keyboardType & inlineKeyboardFlag)) {
    payload["message_id"] = message.backMessageId;
  }
  JsonObject& replyMarkup = payload.createNestedObject("reply_markup");
  if (keyboardType & inlineKeyboardFlag) {
    replyMarkup["inline_keyboard"] = keyboardBuffer;
  } else {
    replyMarkup["keyboard"] = keyboardBuffer;
  }
  //Telegram defaults these values to false, so to decrease the size of the payload we will only send them if needed
  if (keyboardType & resizeFlag) {
    replyMarkup["resize_keyboard"] = true;
  }
  if (keyboardType & oneTimeFlag) {
    replyMarkup["one_time_keyboard"] = true;
  }
  if (keyboardType & selectiveFlag) {
    replyMarkup["selective"] = true;
  }
  return sendPostMessage(payload);
}
  
bool TelegramBotAPI::sendChatAction( ActionType action) const {
  DEBUGV(":SendChatAction\n");
  String command;
  switch (action) {
    case upload_photo:    command = "upload_photo"; break;
    case record_video:    command = "record_video"; break;
    case upload_video:    command = "upload_video"; break;
    case record_audio:    command = "record_audio"; break;
    case upload_audio:    command = "upload_audio"; break;
    case upload_document: command = "upload_document"; break;
    case find_location:   command = "find_location"; break;
    case typing:          command = "typing"; break;
    default:              return false;
  }
  command = "/sendChatAction?chat_id=" + String(message.chatId) + "&action=" + command;
  return sendGetMessage(command);
}
  
// Privates: 
bool TelegramBotAPI::sendPostMessage(const JsonObject& payload) const {
  DEBUGV(":SendPostMessage ");
  const unsigned long sttime = millis();
  if (payload.containsKey("text")) {
    while (millis() < sttime + sendDelay_) { // loop for a while to send the message
      sendPostToTelegram(payload);
      const String data = readResponse(maxResponse_);
      if (checkResponse(data)) {
        return true;
      }
    }
  }
  return false;
}
  
bool TelegramBotAPI::sendGetMessage(const String& getCommand) const {
  DEBUGV(":SendGetMessage ");
  const unsigned long sttime = millis();
  while (millis() < sttime + sendDelay_) {    // loop for a while to send the message
    const String command ("bot" + _token + getCommand);
    sendGetToTelegram(command);
    const String response = readResponse(maxResponse_);
    if (checkResponse(response)) {
      return true;
    }
  }
  return false;
}

void TelegramBotAPI::sendMediaToTelegram (const String & command, const String & binaryProperyName, 
                                          const String & fileName, const String & contentType ) const {
  DEBUGV(":sendMediaToTelegram ");
  File file = SD.open(fileName, FILE_READ);
  if (!file) {
    DEBUGV("Error to open %s\n", fileName.c_str());
    return ; 
  }
  String boundry = "------------------------b8f610217e83e29b";
  for (byte i=0; i < tryCount_; ++i) {
    if (_client.connected() || _client.connect(host_, sslPort_)) {
      i = tryCount_;
      String start_request = "--" + boundry + "\r\n" 
          + "content-disposition: form-data; name=\"chat_id\"\r\n\r\n"
          + message.chatId + "\r\n"
          + "--" + boundry + "\r\n"
          + "content-disposition: form-data; name=\"" + binaryProperyName + "\"; filename=\"" + fileName + "\"\r\n"
          + "Content-Type: " + contentType + "\r\n\r\n" 
          ;
      String end_request = "\r\n--" + boundry + "--\r\n";
      _client.print("POST /bot"+_token+"/" + command); _client.println(" HTTP/1.1");
      // Host header
      _client.print("Host: "); _client.println(host_);
      _client.println("User-Agent: arduino/1.0");
      _client.println("Accept: */*");
      const unsigned int contentLength = file.size() + start_request.length() + end_request.length();
      DEBUGV("Content-Length: %d\n", contentLength);
      _client.print("Content-Length: "); _client.println(String(contentLength));
      _client.println("Content-Type: multipart/form-data; boundary=" + boundry);
      _client.println("");
      _client.print(start_request);
      DEBUGV (start_request.c_str());
      while (file.available() > 0) {
        uint8_t buffer[bufferSize_];
        const unsigned int readed = file.read(buffer, bufferSize_);
        _client.write(buffer, readed);
      }
      _client.print(end_request);
      DEBUGV (end_request.c_str());
    } else {
      delay(10);
      DEBUGV(" Connect error! ");
    }
  }
}

void TelegramBotAPI::sendPostToTelegram(const JsonObject& payload) const {
  DEBUGV(":SendPostLight ");
  if (payload.containsKey("text")) {
    for (byte i = 0; i< tryCount_; ++i)
	    if (_client.connected() || _client.connect(host_, sslPort_)) {
        i = tryCount_;
        String data;
        payload.printTo(data);
        String head = "POST /bot" + _token + (payload.containsKey("message_id") ? "/editMessageText" : "/sendMessage") + " HTTP/1.1"
             + " \r\nHost:" + host_ 
             + " \r\nContent-Type: application/json"
             + " \r\nContent-Length:" + String(data.length()) ;
        _client.println(head);
        _client.println();
        _client.println(data);
        //DEBUGV(head.c_str()) ;
        //DEBUGV("\n") ;
        //DEBUGV(data.c_str()) ;
      } else {
        delay(10);
        DEBUGV(" Connect error! ");
      }
  } else {
      DEBUGV(" No 'text' ");
  }
}

void TelegramBotAPI::sendGetToTelegram(const String & command) const {
  DEBUGV(":SendGetToTelegram: %s \n", command.c_str());
	// Connect with api.telegram.org
  for (byte i=0; i<tryCount_; ++i)
  	if (_client.connected() || _client.connect(host_, sslPort_)) {
      i = tryCount_;
  		DEBUGV("Connect OK.\n");
  		_client.println("GET /" + command);
  	} else {
      delay(10);
      DEBUGV("Connect ERROR!\n");
    }
}

String TelegramBotAPI::readResponse(unsigned int limit) const {
  DEBUGV("\n:ReadResponse ");
  String response (""); // response.reserve(maxMessageLength_) ;
  const unsigned long now = millis();
  unsigned int count = 0;
  bool avail = false;
  while (millis() - now < readDelay_) {
    while (_client.available() > 0) {
      const char c = _client.read();
      if (count < limit) {
        response += c;
        ++count;
      }
      avail = true;
    }
    if (avail) {
      break;
    }
  }
  DEBUGV(" length: %d\n", response.length());
  DEBUGV("%s\n\n", response.c_str());
  return response;
}

int TelegramBotAPI::getChatId(const String & response) const {
  return getIntVal( response, "\"chat\":{\"id\":") ;
}

int TelegramBotAPI::getOffset(const String & response) const {
  return getIntVal( response, "\"update_id\":") ;
}

int TelegramBotAPI::getMessageId(const String & response) const {
  return getIntVal( response, "\"message_id\":") ;
}

int TelegramBotAPI::getIntVal(const String & response, const String & findStr) const {
  DEBUGV(":GetIntVal %s; ", findStr.c_str());
  int i = response.indexOf(findStr);
  if (i < 0) return 0;
  i += findStr.length();
  int j = response.indexOf(',', i);
  if (j < i) return 0;
  const String strVal = response.substring(i, j);
  DEBUGV("IntVal=%s\n", strVal.c_str());
  return strVal.toInt();
}

bool TelegramBotAPI::checkResponse(const String & response) const {
  DEBUGV(":СheckResponse %s\n", response.c_str());
  const String findStr =  "\"ok\":" ;
  int i = response.indexOf(findStr);
  if (i < 1) return false;
  else return true; 
  // "ok":true is correct response. May be "ok":false, but we do not have error handling.
}

String TelegramBotAPI::convertFromUnicode(const String & unicodeStr) const {
  DEBUGV (":ConvertUnicode ");
  const int len = unicodeStr.length();
  String out; 
  for (int i = 0; i < len; ++i) {
     char iChar = unicodeStr[i];
     if (iChar == '\\') { // got escape char
       iChar = unicodeStr[++i];
       if(iChar == 'u') { // got unicode hex
         ++i; int j = i + 4;
         const String unicode = unicodeStr.substring(i, j);
         DEBUGV("; %s ->", unicode.c_str()) ;
         i = j-1; char * error = NULL;
         unsigned long unicodeVal = strtol(unicode.c_str(), &error, 16);
         /*
         АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя
         unicode 0410 - 043F, 0440 - 044F  0401  0451
         utf-8   D090 - D0BF, D180 - D18F  D081  D191
         */
         if (unicodeVal == 0x0401) {
           unicodeVal = 0xD081;
         } else if (unicodeVal == 0x0451) {
           unicodeVal = 0xD191;
         } else if ((unicodeVal >= 0x0410) && (unicodeVal <= 0x043f)) {
           unicodeVal += 0xCC80;
         } else if ((unicodeVal >= 0x0440) && (unicodeVal <= 0x044f)) {
           unicodeVal += 0xCD40;
         }
         char * c = (char*) &unicodeVal;
         j = c[0] ; c[0] = c[1]; c[1] = j; 
         out += c;
         DEBUGV (c);
       } else if (iChar == 'n') {
         out += '\n';
       } else {
         out += iChar;
       }
     } else {
       out += iChar;
     }
  }
  DEBUGV(". End convert\n");
  return out;
}

String TelegramBotAPI::convertToUnicode(const String & data) const {
  DEBUGV (":ConvertUnicode ");
  const int len = data.length();
  String out; 
  for (int i = 0; i < len; ++i) {
     unsigned int unicodeVal = data[i];
     if (0xD0 == unicodeVal || 0xD1 == unicodeVal) {
       unicodeVal = (unicodeVal << 8) + data[++i];
         /*
         АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя
         unicode 0410 - 043F, 0440 - 044F  0401  0451
         utf-8   D090 - D0BF, D180 - D18F  D081  D191
         */
       if (unicodeVal == 0xD081) {
         unicodeVal = 0x0401;
       } else if (unicodeVal == 0xD191) {
         unicodeVal = 0x0451;
       } else if ((unicodeVal >= 0xD090) && (unicodeVal <= 0xD0BF)) {
         unicodeVal -= 0xCC80;
       } else if ((unicodeVal >= 0xD180) && (unicodeVal <= 0xD18F)) {
         unicodeVal -= 0xCD40;
       }
       String c = String(unicodeVal, HEX);
       while (c.length() < 4) {
         c = "0" + c;
       }
       c = "\\u" + c ;
       out += c;
       DEBUGV("%c%c to %s; ", data[i-1], data[i], c.c_str());
     } else {
       out += (char)unicodeVal;
     }
  }
  DEBUGV(". End convert\n");
  return out;
}

bool TelegramBotAPI::checkMessage() const {
  bool isEmpty = true;
  for (unsigned int i = 0; i < message.text.length(); ++i) {
    const char m = message.text[i];
    if (m == 0xD0 || m == 0xD1) {
      ++i;
      isEmpty = false;
    } else if (m < 0x20 && m != '\n' && m != '\r') {
      DEBUGV("Error: %c\n", m);
      return false;
    } else if (m > 0x7E) {
      DEBUGV("Error: %c\n", m);
      return false;
    } else if (m > 0x20 && m < 0x7F) {
      isEmpty = false;      
    }
  }
  if (isEmpty) DEBUGV("Mess is empty\n");
  return !isEmpty;
}
/*
String TelegramBotAPI::sendPostPhoto(const JsonObject& payload)  {
  bool sent = false;
  String response = "";
  if (debug) Serial.println("SEND Post Photo");
  const unsigned long sttime = millis();
  if (payload.containsKey("photo")) {
    while (millis() < sttime + sendDelay_) { // loop for a while to send the message
      const String command = "bot" + _token + "/sendPhoto";
      response = sendPostToTelegram(command, payload);
      if (debug) Serial.println(response);
      sent = checkForOkResponse(response);
      if (sent) {
        break;
      }
    }
  }
  return response;
}

String TelegramBotAPI::sendPhotoByBinary(const String & chat_id, const String & contentType, int fileSize,
    MoreDataAvailable moreDataAvailableCallback, GetNextByte getNextByteCallback) {
  if (debug) Serial.println("SEND Photo");
  const String response = sendMultipartFormDataToTelegram("sendPhoto", "photo", "img.jpg",
    contentType, chat_id, fileSize, moreDataAvailableCallback, getNextByteCallback);
  if (debug) Serial.println(response);
  return response;
}

String TelegramBotAPI::sendPhoto(const String & chat_id, const String & photo, const String & caption, 
                                       bool disable_notification, int reply_to_message_id, const String & keyboard) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& payload = jsonBuffer.createObject();
  payload["chat_id"] = chat_id;
  payload["photo"] = photo;
  if (caption) {
    payload["caption"] = caption;
  }
  if (disable_notification) {
    payload["disable_notification"] = disable_notification;
  }
  if (reply_to_message_id && reply_to_message_id != 0) {
    payload["reply_to_message_id"] = reply_to_message_id;
  }
  if (keyboard) {
    JsonObject& replyMarkup = payload.createNestedObject("reply_markup");
    DynamicJsonBuffer keyboardBuffer;
    replyMarkup["keyboard"] = keyboardBuffer.parseArray(keyboard);
  }
  return sendPostPhoto(payload);
}

bool TelegramBotAPI::getMe() {
  const String command = "bot" + _token + "/getMe";
  const String response = sendGetToTelegram(command); //receive reply from telegram.org
  DynamicJsonBuffer jsonBuffer;
  const JsonObject & root = jsonBuffer.parseObject(response);
  if(root.success()) {
    if (root.containsKey("result")) {
      const String _name = root["result"]["first_name"];
      const String _username = root["result"]["username"];
      name = _name;
      userName = _username;
      return true;
    }
  }
  return false;
}
*/
