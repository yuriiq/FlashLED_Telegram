
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Initialize Wifi connection to the router
#define SSID "DIR-615"     // your network SSID (name)
#define PASS "80129820"    // your network key

// Initialize Telegram BOT
#define BOTtoken "316201411:AAHL3pewSswc7-XHGcjmhx3gyu5DdvSSNdU"  // your Bot Token (Get from Botfather)

void handleMessage (const JsonObject& message);
void handleError   (int callback);

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

const unsigned int bot_mtbs =  3000; // mean time between scan messages
unsigned long bot_lasttime = 0; // last time messages' scan has been done

const int ledPin = LED_BUILTIN;
bool ledStatus = false;

void ledOnCommand () {
    ledStatus = true;
    digitalWrite(ledPin, LOW);   // turn the LED on (HIGH is the voltage level)
    inlineKeyboardCommand("Диод ВКЛ"); 
}

void ledOffCommand () {
    ledStatus = false;
    digitalWrite(ledPin, HIGH);    // turn the LED off (LOW is the voltage level)
    inlineKeyboardCommand("Диод ВЫКЛ"); 
}

void statusCommand () {
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
    JsonArray& rowCommand = keyboard.createNestedArray();
    JsonObject& startButton = rowCommand.createNestedObject();
    startButton["text"] = "Приветствие";
    startButton["callback_data"] = "/start";
    JsonObject& statusButton = rowCommand.createNestedObject();
    statusButton["text"] = "Статус";
    statusButton["callback_data"] = "/status";
    if (ledStatus) {
        JsonObject& ledoffButton = rowCommand.createNestedObject();
        ledoffButton["text"] = "Выключить";
        ledoffButton["callback_data"] = "/ledoff";
    } else {
        JsonObject& ledonButton = rowCommand.createNestedObject();
        ledonButton["text"] = "Включить";
        ledonButton["callback_data"] = "/ledon";
    }
    JsonArray& linkRow = keyboard.createNestedArray();
    JsonObject& linkButton = linkRow.createNestedObject();
    linkButton["text"] = "Сайт про Telegram на русском";
    linkButton["url"] = "https://tlgrm.ru/docs/bots/api";
    //keyboard.prettyPrintTo(Serial);
    bot.message.text = message;
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

void WiFiReconnect()
{
  // Set WiFi to station mode and disconnect from an AP if it was Previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); delay(100);
  // attempt to connect to Wifi network:
  Serial.print("\nConnecting Wifi: ");
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ") ;
  Serial.println(WiFi.localIP()) ;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
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
}

void handleCommand() {
//  if (command.substring(0,4) == "http") bot.sendPhoto  (chat_id, command, command ); 
  const String & command = bot.message.text;
  const String & fromName = bot.message.from;
  Serial.println("Cmd from " + fromName + ": " + command);
  if (!bot.message.backMessageId) {
      bot.sendChatAction(typing);  
  }
  if (command == "/ledon")       ledOnCommand          ();
  else if (command == "/ledoff") ledOffCommand         ();
  else if (command == "/status") statusCommand         ();      
  else if (command == "/repkb")  replyKeyboardCommand  ();
  else if (command == "/inkb")   inlineKeyboardCommand ("Клавиатура");
  else                           startCommand          (fromName);  
}
