/*******************************************************************
    Телеграм Бот для модуля WeMos D1 R2 c МК ESP8266.
    Предназначен для вкл./выкл. светодиода на плате.
 *******************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Thread.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Имя и пароль вашей сети Wifi 
#define WIFI_SSID "YOURSSID"
#define WIFI_PASSWORD "PWD"
// Телеграм Бот Токен, можно получить у бота @BotFather в Телеграмм 
#define BOT_TOKEN "TOKEN"   
#define toggle_delay 100   

Thread ledThread = Thread(); // создаём поток управления светодиодом
Thread commandThread = Thread(); // создаём поток приема команд

const unsigned long BOT_MTBS = 1000; // Через сколько времени проверять сообщения

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);


char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const long utcOffsetInSeconds = 10800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

unsigned long bot_lasttime; // Последнее время сканирования сообщения

const int ledPin = LED_BUILTIN;
int ledStatus = 0;



void handleNewMessages(int numNewMessages)
{
  Serial.print("Обработка нового сообщения ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/ledon")
    {
      digitalWrite(ledPin, LOW); // Включить светодиод на плате
      ledStatus = 1;
      bot.sendMessage(chat_id, "Светодиод включен - ON", "");
    }

    if (text == "/ledoff")
    {
      ledStatus = 0;
      digitalWrite(ledPin, HIGH); // Выключить светодиод на плате
      bot.sendMessage(chat_id, "Светодиод выключен - OFF", "");
    }
    if (text == "/toggle") {
      ledStatus = 3;
      bot.sendMessage(chat_id, "Светодиод мигает! ", "");
    }

    if (text == "/datetime") {
      bot.sendMessage(chat_id, "Обновление даты...", "");
        while(!timeClient.update()) {
        timeClient.forceUpdate();
       }

       bot.sendMessage(chat_id, "Обновление даты произведено", "");
      String formattedDate = timeClient.getFormattedTime();
      Serial.println(formattedDate);

     String datetime = "Дата: " + String(daysOfTheWeek[timeClient.getDay()]) + "\n Время: " + formattedDate; 
     bot.sendMessage(chat_id, datetime, "Markdown");
    }

    if (text == "/status")
    {
      if (ledStatus==1)
      {
        bot.sendMessage(chat_id, "Светодиод включен - ON", "");
      }
      else if(ledStatus == 0)
      {
        bot.sendMessage(chat_id, "Светодиод выключен - OFF", "");
      }
      else {
         bot.sendMessage(chat_id, "Светодиод мигает", "");
      }
    }

    if (text == "/start")
    {
      String welcome = "Привет, я Телеграм Бот. Я умею преключать светодиод на модуле WeMos D2 R1 " + from_name + ".\n";
      welcome += "А это перечень команд, которые я пока знаю.\n\n";
      welcome += "/ledon : Переключает светодиод в состояние ON\n";
      welcome += "/ledoff : Переключает светодиод в состояние OFF\n";
      welcome += "/status : Возвращает текущее состояние светодиода\n";
      welcome += "/toggle : Переключает светодиод\n";
      welcome += "/datetime : Вывод текущей даты и времени\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}
void command_receiver() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1); 
    while (numNewMessages)
    {
      Serial.println("Получен ответ");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
}

void led_toggle() {
  if(ledStatus == 3) {
      int lighting_mode = digitalRead(ledPin) == HIGH ? LOW : HIGH;
      digitalWrite(ledPin,lighting_mode);
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println();

  ledThread.onRun(led_toggle);  // назначаем потоку задачу
  ledThread.setInterval(toggle_delay); // задаём интервал срабатывания, мсек
    
  commandThread.onRun(command_receiver);     // назначаем потоку задачу
  commandThread.setInterval(20); // задаём интервал срабатывания, мсек

  pinMode(ledPin, OUTPUT); // Настраиваем пин ledPin на выход
  delay(10);
  digitalWrite(ledPin, HIGH); // По умолчанию светодиод выключен

  // attempt to connect to Wifi network:
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  Serial.print("Подключение к сети Wifi SSID: ");
  Serial.print(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi подключен. IP адрес: ");
  Serial.println(WiFi.localIP());

  // Check NTP/Time, usually it is instantaneous and you can delete the code below.
  Serial.print("Время подключения: ");
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  timeClient.begin();
}


unsigned long toggle_millis = 0;

void loop()
{
  if (ledThread.shouldRun())
        ledThread.run(); // запускаем поток
    
    if (commandThread.shouldRun())
        commandThread.run(); // запускаем поток
}