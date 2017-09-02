#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "wifi";
const char* password = "ffffffffee";
//const char* ssid = "nacho router";
//const char* password = "iddibaiddiba";

const int ESP_BUILTIN_LED = LED_BUILTIN;

// declare telnet server (do NOT put in setup())
WiFiServer telnetServer(23);
WiFiClient serverClient;

#include <Ticker.h>

Ticker flipper;

void flip()
{
  int state = digitalRead(LED_BUILTIN);  // get the current state of LED pin
  digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    flipper.attach(1.1 - (progress/total), flip);
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    flipper.attach(0.1, flip);
  });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(ESP_BUILTIN_LED, OUTPUT);

  telnet_setup();
  ir_setup();

  // flip the pin every 0.3s
  flipper.attach(0.3, flip);

}

void loop() {
  ArduinoOTA.handle();
  telnet_loop();
  ir_loop();
/*  digitalWrite(ESP_BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(ESP_BUILTIN_LED, HIGH);
  delay(100);
  */
}
