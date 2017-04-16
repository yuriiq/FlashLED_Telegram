#define DEBUGV(...) Serial.printf (__VA_ARGS__)

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SD.h>
#include <time.h>

#define DEF_SSID "DIR-615"
#define DEF_PASS "80129820"
#define LOGFILE "datalog.txt"
#define SETTINGS_FILE "settings.txt"
#define BOTtoken "316201411:AAHL3pewSswc7-XHGcjmhx3gyu5DdvSSNdU" 


WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

const unsigned int bot_mtbs =  3000; // mean time between scan messages
unsigned long bot_lasttime = 0; // last time messages' scan has been done

const int ledPin = LED_BUILTIN;
bool ledStatus = false;

unsigned int blink = 0;
unsigned long lasttime = 0;

struct Settings {
    String SSID = DEF_SSID;   
    String PASS = DEF_PASS;  
    String toString() {
        return String("SSID:") + SSID + String("\nPASS:") + PASS + '\n';
    }
    void parse(char c) {
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
private:
    String buf;
    enum {NONE, isSSID, isPASS } status = NONE;
};

void logToSD ( const String & msg) {
    File dataFile = SD.open(LOGFILE, FILE_WRITE);
    if (dataFile) {
        dataFile.println(millis() + msg + "\n");
        dataFile.close();
    }
}

void ledOnCommand () {
    blink = 0;
    ledStatus = true;
    digitalWrite(ledPin, LOW);   // turn the LED on (HIGH is the voltage level)
    inlineKeyboardCommand("Диод ВКЛ"); 
}

void ledOffCommand () {
    blink = 0;
    ledStatus = false;
    digitalWrite(ledPin, HIGH);    // turn the LED off (LOW is the voltage level)
    inlineKeyboardCommand("Диод ВЫКЛ"); 
}

void statusCommand () {
    bot.message.backMessageId = 0;
    if(ledStatus){
      bot.message.text = "Диод ВКЛ";
      bot.sendMessage();
    } else {
      bot.message.text = "Диод ВЫКЛ";
      bot.sendMessage();
    }
}

void replyKeyboardCommand() {
    DynamicJsonBuffer jsonBuffer;
    JsonArray& keyboard = jsonBuffer.createArray();
    JsonArray& row1 = keyboard.createNestedArray();
    row1.add ("/ledon");
    row1.add ("/ledoff");
    JsonArray& row2 = keyboard.createNestedArray();
    row2.add ("/status");
    row2.add ("/start");
    keyboard.prettyPrintTo(Serial);
    bot.message.text = "Клавиатура внизу";
    bot.sendMessageWithKeyboard(keyboard, resizeFlag);
}

void inlineKeyboardCommand(const String & message) {
    DynamicJsonBuffer jsonBuffer;
    JsonArray& keyboard = jsonBuffer.createArray();
    {
        JsonArray& rowCommandA = keyboard.createNestedArray();
        JsonObject& startButton = rowCommandA.createNestedObject();
        startButton["text"] = "Приветствие";
        startButton["callback_data"] = "/start";
        JsonObject& statusButton = rowCommandA.createNestedObject();
        statusButton["text"] = "Статус";
        statusButton["callback_data"] = "/status";
        JsonArray& rowCommand = keyboard.createNestedArray();
        JsonObject& ledoffButton = rowCommand.createNestedObject();
        ledoffButton["text"] = "Выключить";
        ledoffButton["callback_data"] = "/ledoff";
        JsonObject& ledonButton = rowCommand.createNestedObject();
        ledonButton["text"] = "Включить";
        ledonButton["callback_data"] = "/ledon";
        JsonObject& blinkButton = rowCommand.createNestedObject();
        blinkButton["text"] = "Мигать";
        blinkButton["callback_data"] = "/blist";
        JsonArray& linkRow = keyboard.createNestedArray();
        JsonObject& linkButton = linkRow.createNestedObject();
        linkButton["text"] = "Сайт про Telegram на русском";
        linkButton["url"] = "https://tlgrm.ru/docs/bots/api";
    }
    //keyboard.prettyPrintTo(Serial);
    bot.message.text = message;
    bot.sendMessageWithKeyboard(keyboard, inlineKeyboardFlag);
}

void bCommand(int i) {
    blink = i;
    inlineKeyboardCommand("Диод мигает каждые " + String(i) + " мс"); 
}

void bListCommand() {
    DynamicJsonBuffer jsonBuffer;
    JsonArray& keyboard = jsonBuffer.createArray();
    {// Saving memory
        JsonArray& rowCommand = keyboard.createNestedArray();
        JsonObject& b1 = rowCommand.createNestedObject();
        b1["text"] = "1 Гц";
        b1["callback_data"] = "/b1";
        JsonObject& b2 = rowCommand.createNestedObject();
        b2["text"] = "2 Гц";
        b2["callback_data"] = "/b2";
        JsonObject& b3 = rowCommand.createNestedObject();
        b3["text"] = "3 Гц";
        b3["callback_data"] = "/b3";
        JsonObject& b4 = rowCommand.createNestedObject();
        b4["text"] = "4 Гц";
        b4["callback_data"] = "/b4";
    }
    bot.message.text = "С какой частотой?";
    bot.sendMessageWithKeyboard(keyboard, inlineKeyboardFlag);
}

void startCommand(const String & from_name) {
    const String welcome = "Привет, " + from_name + ". Я бот для проверки GSM.\n" \
      "Это тестовая программа.\n\n" \
      "/ledon : Включить диод\n" \
      "/ledoff : Выключить диод\n" \
      "/status : Cтатус\n" \
      "/repkb: Клавиатура\n" \
      "/inkb: Встроенная клавиатура\n" \
      ;
    bot.message.text = welcome;
    bot.sendMessage();
}

Settings getSettings()
{
    Settings settings;
    File dataFile = SD.open(SETTINGS_FILE, FILE_READ);
    if (dataFile) {
        String buff;
        while (dataFile.available() > 0) {
            settings.parse(dataFile.read());
        }
        dataFile.close();
    } else {
        DEBUGV("Error to load %s\n", SETTINGS_FILE);    
    }
    const String msg = String("Settings in ") + SETTINGS_FILE +"\n"+ settings.toString();
    Serial.println(msg);
    logToSD(msg);
    return settings;
}

void WiFiReconnect()
{
  // Set WiFi to station mode and disconnect from an AP if it was Previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); delay(100);
  // attempt to connect to Wifi network:
  const Settings settings = getSettings();
  DEBUGV("\nConnecting Wifi: ");
  WiFi.begin(settings.SSID.c_str(), settings.PASS.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    DEBUGV("."); delay(500);
  }
  const String msg = "\nWiFi connected. IP address:" + WiFi.localIP().toString();
  DEBUGV("%s\n", msg.c_str()) ;
  logToSD (msg) ;
}

void handleCommand() {
//  if (command.substring(0,4) == "http") bot.sendPhoto  (chat_id, command, command ); 
  const String & command = bot.message.text;
  const String & fromName = bot.message.from;
  const String msg = "Cmd from " + fromName + ": " + command + "\n";
  DEBUGV(msg.c_str());
  logToSD(msg);
  
  if (!bot.message.backMessageId) {
      bot.sendChatAction(typing);  
  }
  if (command == "/ledon")       ledOnCommand          ();
  else if (command == "/ledoff") ledOffCommand         ();
  else if (command == "/status") statusCommand         ();      
  else if (command == "/repkb")  replyKeyboardCommand  ();
  else if (command == "/inkb")   inlineKeyboardCommand ("Клавиатура");
  else if (command == "/blist")  bListCommand(); 
  else if (command == "/b1")     bCommand(1000) ;
  else if (command == "/b2")     bCommand(500);
  else if (command == "/b3")     bCommand(333);
  else if (command == "/b4")     bCommand(250);
  else                           startCommand          (fromName);  
}

void setupSD() {
  if (SD.begin(D10)) {
    File dataFile = SD.open(LOGFILE, FILE_WRITE);
    if (dataFile) {
        dataFile.println(String(time(NULL)) + ": Program start.");
        dataFile.close();
        DEBUGV("card initialized.\n");
    }
    dataFile = SD.open(SETTINGS_FILE, FILE_READ);
    if (dataFile) {
        Serial.println(SETTINGS_FILE);
        while (dataFile.available() > 0) {
            Serial.write(dataFile.read());
        }
        dataFile.close();
    } else {
        dataFile = SD.open(SETTINGS_FILE, FILE_WRITE);
        Settings settings; 
        dataFile.println(settings.toString());
        dataFile.close();
    }
  } else {
    DEBUGV("Card failed, or not present\n");
  }
}
    
void setup() {
  Serial.begin(115200);
 // Serial.setDebugOutput(true);
  pinMode(ledPin, OUTPUT); // initialize digital ledPin as an output.
  delay(10);
  digitalWrite(ledPin, HIGH); // initialize pin as off
  Serial.println("\bool = " + String(sizeof(bool)) 
               + "; char = " + String(sizeof (char)) 
               + "; wchar_t = " + String(sizeof (wchar_t)) 
               + "; byte = " + String(sizeof (byte)) 
               + "; short = " + String(sizeof (short)) 
               + "; int = " + String(sizeof (int)) 
               + "; long = " + String(sizeof (long)) 
               + "; long long = " + String(sizeof (long long))) ;
  setupSD();
}

void loop() {
  if (millis() > bot_lasttime + bot_mtbs)  {
    if (WL_CONNECTED == WiFi.status()) {
      while (bot.getUpdates ()) {
        handleCommand();
      }
      bot_lasttime = millis();
    } else WiFiReconnect();
  }
  if (blink && (millis() > lasttime + blink)) {
    lasttime = millis();
    if (ledStatus) {
        ledStatus = false;
        digitalWrite(ledPin, HIGH);  
    } else {
        ledStatus = true;
        digitalWrite(ledPin, LOW);  
    }
  }
}

