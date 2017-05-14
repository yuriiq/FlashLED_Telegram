#define DEBUGV(...) Serial.printf (__VA_ARGS__)

#include "TelegramBotAPI.h"
#include "SDCardRW.h"
// #include "gsm.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>


#define SETTINGS_FILE "settings.txt"
#define BOTtoken "316201411:AAHL3pewSswc7-XHGcjmhx3gyu5DdvSSNdU" 
#define logFile  "datalog.txt"
#define recFile  "test2.wav"

WiFiClientSecure client;
TelegramBotAPI bot(BOTtoken, client);
// GSM gsm(D8, D9);

const unsigned int bot_mtbs =  3000; // mean time between scan messages
unsigned long bot_lasttime = 0; // last time messages' scan has been done

void startCommand(const String & from_name) {
    const String welcome = "Привет, " + from_name + ". Я бот для проверки GSM.\n" \
      "Это тестовая программа.\n\n" \
      "/balance - узнать баланс\n"
      "/info - служебная информация\n"
      "/call <номер телефона> <время ожидания> - дозвон на номер"
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
  const Settings settings = SDCardRW.getSettings(SETTINGS_FILE);
  DEBUGV("\nConnecting Wifi: ");
  WiFi.begin(settings.SSID.c_str(), settings.PASS.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    DEBUGV("."); delay(500);
  }
  SDCardRW.logToSD ("\nWiFi connected. IP address:" + WiFi.localIP().toString(), logFile) ;
}

void infoCommand() {
  bot.message.text = SDCardRW.infoFile(recFile);
  bot.sendMessage();
}

void balanceCommand() {
//    bot.message.text = gsm.balance();
    bot.sendMessage();
}

void callCommand(const String & command) {
  const String errorMsg = "Формат команды:\n/call <номер телефона> <время ожидания>";
  const int index = command.indexOf(' ');
  if (index < 0) {
    bot.message.text = errorMsg;
    bot.sendMessage();
    return;
  }
  const int indexTime = command.indexOf(' ', index + 1);
  if (indexTime < 0) {
      bot.message.text = errorMsg;
      bot.sendMessage();
      return;
  }
  const String tell = command.substring(index, indexTime);
  int timeout = command.substring(indexTime).toInt();
  bot.message.text = String("Дозвон по номеру " + tell+ ", время ожидания: "+ timeout+ " сек.");
  bot.sendMessage();
  handleRecord(timeout*1000);
}

void handleCommand() {
//  if (command.substring(0,4) == "http") bot.sendPhoto  (chat_id, command, command ); 
  const String & command = bot.message.text;
  const String & fromName = bot.message.from;
  SDCardRW.logToSD("Cmd from " + fromName + ": " + command + "\n", logFile);
  if (!bot.message.backMessageId) {
      bot.sendChatAction(typing);  
  }
  if (command == "/start") startCommand (fromName);  
  else if (command == "/balance") balanceCommand();
  else if (command == "/info") infoCommand();
  else if (command.startsWith("/call")) callCommand(command);
  // else sendCommand  (bot.message.text);  
}

void handleRecord(unsigned int timeout) {
  SDCardRW.startRec(recFile);
  DEBUGV("start rec: %d\n", millis());
  delay(timeout);
  SDCardRW.stopRec();
  DEBUGV("stopRec\n");
  bot.message.text = "Дозвон окончен.";
  bot.sendMessage();
  bot.sendAudio(recFile); 
}
    
void setup() {
  Serial.begin(9600);
  SDCardRW.setupSD();
//  gsm.begin(9600);
}

void loop() {
  if ((millis() > bot_lasttime + bot_mtbs))  {
    if (WL_CONNECTED == WiFi.status()) {
      if (bot.getUpdates ()) {
        handleCommand();
      }
      bot_lasttime = millis();
    } else WiFiReconnect();
  } 
}

