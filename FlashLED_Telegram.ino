#include "settings.h"
#include "TelegramBotAPI.h"
#include "SDCardRW.h"
#include "gsm.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

WiFiClientSecure client;
TelegramBotAPI bot(BOTtoken, client);
GSM gsm(D8, D9);

const unsigned int bot_mtbs =  3000; // mean time between scan messages
unsigned long bot_lasttime = 0; // last time messages' scan has been done

void startCommand(const String & from_name) {
  const String welcome = "Привет, " + from_name + ". Я бот для проверки GSM.\n" \
                         "Это тестовая программа.\n\n" \
                         "/balance - узнать баланс\n"
                         "/info - служебная информация\n"
                         "/call <номер> <время> - дозвон на номер"
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
  const Settings settings = SDCardRW.getSettings(settingsFileName);
  DEBUGV("\nConnecting Wifi: ");
  WiFi.begin(settings.SSID.c_str(), settings.PASS.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    DEBUGV("."); delay(500);
  }
  DEBUGV("%s\n", WiFi.localIP().toString().c_str()); 
}

void handleGSMMessage(const String & message) {
  /*
  if (message.indexOf("NO DIALTONE") >= 0)     bot.message.text = "Нет сигнала";
  else if (message.indexOf("BUSY") >= 0)       bot.message.text = "Вызов отклонён";
//  else if (message.indexOf("NO CARRIER") >= 0) bot.message.text = "Повесили трубку";
  else if (message.indexOf("NO ANSWER") >= 0)  bot.message.text = "Нет ответа";
  else return;
  bot.sendMessage();
  */
}

void infoCommand() {
  bot.message.text = SDCardRW.infoFile(recFile);
  bot.sendMessage();
}

void balanceCommand() {
  bot.message.text = gsm.balance();
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
  unsigned int timeout = command.substring(indexTime).toInt();
  bot.message.text = String("Попытка дозвона на " + tell + ", ожидание: " + timeout + " сек");
  bot.sendMessage();
  SDCardRW.startRec(recFile);
  DEBUGV("startRec\n" );
  const String msg = gsm.call(tell, timeout*1000);
  DEBUGV("stopRec\n");
  SDCardRW.stopRec();
  bot.message.text = "Дозвон окончен";
  bot.sendMessage();
  recSendCommand();
  handleGSMMessage(msg);
}

void recSendCommand() {
  if (!bot.sendAudio(recFile, SDCardRW.recSize (recFile))) {
    delay(10);
    bot.message.text = "Файл не загружен. Повтор: /rec";
    bot.sendMessage();
  }
}

void handleCommand() {
  //  if (command.substring(0,4) == "http") bot.sendPhoto  (chat_id, command, command );
  const String & command = bot.message.text;
  const String & fromName = bot.message.from;
  if (!bot.message.backMessageId) {
    bot.sendChatAction(typing);
  }
  if (command == "/start") startCommand (fromName);
  else if (command == "/balance") balanceCommand();
  else if (command == "/info") infoCommand();
  else if (command.startsWith("/call")) callCommand(command);
  else if (command.startsWith("/rec")) recSendCommand();
  // else sendCommand  (bot.message.text);
}

void setup() {
  const int speed = 4800;
  Serial.begin(speed);
  SDCardRW.setupSD();
  gsm.begin(speed);
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

