//https://forum.arduino.cc/t/how-to-send-http-response-with-json-data-from-arduino-to-web-browser/337428/4
#include <Arduino.h>

// Load Wi-Fi library
#include <ESP8266WiFi.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Adafruit_AM2320.h"

#include <SPI.h>
#include <Wire.h>

// Replace with your network credentials
// const char *ssid = "zeone";
// const char *password = "Zeone1989";
const char *ssid = "Zeone_IoT";
const char *password = "Zeone1989";


// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

unsigned long tempTiming;
unsigned long humTiming;

bool ledIsOn = false;

Adafruit_AM2320 am2320 = Adafruit_AM2320();

float temp = 0;
float hum = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("Booting"); //  "Загрузка"

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    //  "Соединиться не удалось! Перезагрузка..."
    delay(5000);
    ESP.restart();
  }

  // строчка для номера порта по умолчанию
  // можно вписать «8266»:
  // ArduinoOTA.setPort(8266);

  // строчка для названия хоста по умолчанию;
  // можно вписать «esp8266-[ID чипа]»:
  ArduinoOTA.setHostname("THD_LivingRoom");

  // строчка для аутентификации
  // (по умолчанию никакой аутентификации не будет):
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]()
                     {
                       Serial.println("Start"); //  "Начало OTA-апдейта"
                     });
  ArduinoOTA.onEnd([]()
                   {
    Serial.println("\nEnd");  //  "Завершение OTA-апдейта" 
    });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { 
                          Serial.printf("Progress: %u%%\r", (progress / (total / 100))); 
                        });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                         Serial.println("Auth Failed");
                       //  "Ошибка при аутентификации"
                       else if (error == OTA_BEGIN_ERROR)
                         Serial.println("Begin Failed");
                       //  "Ошибка при начале OTA-апдейта"
                       else if (error == OTA_CONNECT_ERROR)
                         Serial.println("Connect Failed");
                       //  "Ошибка при подключении"
                       else if (error == OTA_RECEIVE_ERROR)
                         Serial.println("Receive Failed");
                       //  "Ошибка при получении данных"
                       else if (error == OTA_END_ERROR)
                         Serial.println("End Failed");
                       //  "Ошибка при завершении OTA-апдейта"
                     });
  ArduinoOTA.begin();
  Serial.println("Ready");      //  "Готово"
  Serial.print("IP address: "); //  "IP-адрес: "
  Serial.println(WiFi.localIP());
  server.begin();
  //Wire.begin();
  //am2320.begin();

  //Wire.pins(0, 2);
  Wire.begin(0, 2);
  am2320.begin();
}

void ShowTemp(WiFiClient client)
{
  client.println(F("Chip = AM2320"));

  client.println("Temperature: ");
  client.print(temp);
  client.print("C\n");

  client.println("Hum: ");
  client.print(hum);
  client.print("%\n");
}

void GetTepm()
{
  if (millis() - tempTiming > 1000)
  {
    tempTiming = millis();
    float currTemp = am2320.readTemperature();
    if (!isnan(currTemp))
      temp = currTemp;
  }

  if (millis() - humTiming > 3000)
  {
    humTiming = millis();
    float currHum = am2320.readHumidity();
    if (!isnan(currHum))
      hum = currHum;
  }
}

void loop()
{
  ArduinoOTA.handle();

  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  {                                // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        {
          // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP8266 Web Server</h1>");

            client.println("<h2>IP-address: " + WiFi.localIP().toString() + "</h2>");
            ShowTemp(client);

            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  GetTepm();
}

// void setup()
// {
//   Serial.begin(9600);
//   Serial.println();

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();
//   delay(100);
// }

// void loop()
// {
//   Serial.print("Scan start ... ");
//   int n = WiFi.scanNetworks();
//   Serial.print(n);
//   Serial.println(" network(s) found");
//   for (int i = 0; i < n; i++)
//   {
//     Serial.println(WiFi.SSID(i));
//   }
//   Serial.println();

//   delay(5000);
// }