#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid = "";
const char *password = "";

unsigned long last_time_1 = 0; // Для крапок підключення Wi-Fi
unsigned long last_time_2 = 0; // Для таймауту підключення
unsigned long last_time_3 = 0; // Для щосекундного виведення часу

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 2 * 3600, 60000); // Змінено зміщення на 2 години, інтервал оновлення 60с

enum States
{
  CONNECTING,
  CONNECTED,
  NOT_CONNECTED
};

States cur_state = CONNECTING;

void setup()
{
  Serial.begin(115200); // Змінено на стандартну швидкість 115200 для чіткості виводу
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  timeClient.begin();

  last_time_2 = millis(); // Запускаємо відлік таймауту
}

int counter = 0;

void loop()
{
  unsigned long cur_time = millis();

  switch (cur_state)
  {
  case CONNECTING:
    // Не блокуємо! Перевіряємо статус Wi-Fi на кожній ітерації loop()
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println();
      Serial.println("Connected successfully!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      cur_state = CONNECTED;
      last_time_3 = cur_time; // ініціалізуємо таймер виведення часу
    }
    else
    {
      // Виводимо крапку кожні 500 мс без зависання
      if (cur_time - last_time_1 >= 500)
      {
        last_time_1 = cur_time;
        Serial.print(".");
      }

      // Перевірка таймауту підключення (30 секунд)
      if (cur_time - last_time_2 >= 30000)
      {
        Serial.println("\nConnection TIMEOUT reached!");
        cur_state = NOT_CONNECTED;
        last_time_2 = cur_time;
      }
    }
    break;

  case NOT_CONNECTED:
    // Очікуємо 5 секунд перед перезавантаженням
    if (cur_time - last_time_2 >= 5000)
    {
      Serial.println("Restarting ESP8266...");
      ESP.restart();
    }
    break;

  case CONNECTED:
    // Перевірка втрати зв'язку під час роботи
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi connection lost! Reconnecting...");
      WiFi.reconnect();
      cur_state = CONNECTING;
      last_time_2 = cur_time;
      break;
    }

    // Виконуємо дію раз на 1 секунду без блокування
    if (cur_time - last_time_3 >= 1000)
    {
      last_time_3 = cur_time; // Оновлюємо мітку часу!
      counter += 1;

      // Оновлюємо NTP не частіше ніж раз на 5 секунд (або рідше) безпечним методом
      if (counter % 5 == 0)
      {
        bool updated = timeClient.update();
        if (updated)
        {
          Serial.println(" [NTP Updated]");
        }
      }

      Serial.print(timeClient.getFormattedTime());
      if (timeClient.isTimeSet())
        Serial.println(" update");
      else
        Serial.println(" not update");
    }

    break;

  default:
    break;
  }
}