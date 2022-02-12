#include<Arduino.h>
#include<ESP8266WiFi.h>
#include<Hash.h>
#include<ESPAsyncTCP.h>
#include<ESPAsyncWebServer.h>
#include<DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include "index.h"
#include "logout.h"

//#define WIFI_SSID "Orange_Swiatlowod_8EB0"
//#define WIFI_PASSWORD "HeqYJd9CE5pdTMFhWq"
#define WIFI_SSID "iPhone"
#define WIFI_PASSWORD "qwerty123"
#define HTTP_USERNAME "admin"
#define HTTP_PASSWORD "admin"
//#define MQTT_HOST IPAddress(192,168,1,16)
#define MQTT_HOST IPAddress(172,20,10,4)
#define MQTT_PORT 1883
#define MQTT_PUB_TEMP "esp/dht/temperature"
#define MQTT_PUB_HUM "esp/dht/humidity"
#define MQTT_PUB_PRES "esp/bmp/pressure"
#define MQTT_PUB_GAS "esp/mq2/gas"
#define MQTT_PUB_ALT "esp/bmp/altitude"
#define MQTT_PUB_REL "esp/relay/lamp"
#define PARAM_INPUT_1 "state"
//DHT11 definitions
#define DHTPIN 14
#define DHTTYPE DHT11
#define RELAY 13
//const int relay = 13;
//const int output = 13; 

float temperature = 0.0;
float humidity = 0.0;
int gas = 0;
float pressure = 0.0;
int altitude = 0;
int Gas_analog = A0;   
unsigned long previousMillis = 0;
unsigned long interval = 10000;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
AsyncWebServer server(80);
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

String relayState(){
  if(digitalRead(RELAY)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

String processor(const String& var){
  if(var == "TEMPERATURE"){
    return String(temperature);
  }
  else if(var == "HUMIDITY"){
    return String(humidity);
  }
  else if(var == "GAS_LEVEL"){
    return String(gas);
  }
  else if(var == "PRESSURE"){
    return String(pressure);
  }
  else if(var == "ALTITUDE"){
    return String(altitude);
  }
  else if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String relayStateValue = relayState();
    buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + relayStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  else if(var == "STATE"){
    if(digitalRead(RELAY)){
      return "ON";
    }
    else {
      return "OFF";
    }
  }
  return String();
}

void connectToWifi() {
  Serial.printf("\nConnecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
 
void connectToMqtt() {
  Serial.printf("\nConnecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.printf("\nConnected to Wi-Fi.");
  Serial.println(WiFi.localIP());
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.printf("\nDisconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.printf("\nConnected to MQTT.");
  Serial.printf("\nSession present: %s", sessionPresent ? "true" : "false");
  uint16_t packetIdSub1  = mqttClient.subscribe(MQTT_PUB_REL, 1);
  Serial.printf("\nSubscribing on topic %s, packetId: %i\n", MQTT_PUB_PRES, packetIdSub1);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("\nDisconnected from MQTT.");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.printf("\nPublish acknowledged - packetId: %i\n", packetId);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.printf("\nSubscribe acknowledged - packetId: %i, qos: %i\n", packetId, qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.printf("\nUnsubscribe acknowledged - packetId: %i\n", packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.printf("\nMessage - topic: %s, payload: %s\n", topic, payload);
  char newPayload[len+1];
  newPayload[len] = '\0';
  strncpy(newPayload, payload, len);
  Serial.printf("\nNew payload: %s\n", newPayload);
  if((String(topic) == MQTT_PUB_REL) && (String(newPayload) =="ON")){
    digitalWrite(RELAY, LOW);
  }
  else{
    digitalWrite(RELAY, HIGH);
  }
}
void setup(){
  Serial.begin(9600); //115200
  pinMode(RELAY, OUTPUT);
  //digitalWrite(output, HIGH);

  //DHT sensor initialization
  dht.begin();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  delay(500);

  //BMP sensor initialization
  if(!bmp.begin()){
    Serial.printf("\nCould not find BMP80");
    while(1);
    }
    bmp.begin();
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500
      );
    
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

  // Print ESP8266 Local IP Address for HTTP server purposes
  Serial.println(WiFi.localIP());


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(HTTP_USERNAME, HTTP_PASSWORD))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
  });
  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", logout_html, processor);
  });
  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if(!request->authenticate(HTTP_USERNAME, HTTP_PASSWORD))
      return request->requestAuthentication();
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      digitalWrite(RELAY, inputMessage.toInt());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperature).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humidity).c_str());
  });
  server.on("/gas_level", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(gas).c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(pressure).c_str());
  });
  server.on("/altitude", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(altitude).c_str());
  });
  
 server.begin();

}
 
void loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float newTemperature = dht.readTemperature();
    // if temperature read failed, don't change 'temperature' variable value
    if (isnan(newTemperature)) {
      Serial.println("Failed to read from DHT11 sensor!");
    }
    else {
      temperature = newTemperature;
      uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temperature).c_str());
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_TEMP, packetIdPub1);
      Serial.printf("\nCurrent temperature value: %.2f\n", temperature);                          
    }
    // Read Humidity
    float newHumidity = dht.readHumidity();
    // if humidity read failed, don't change 'humidity' variable value 
    if (isnan(newHumidity)) {
      Serial.println("Failed to read from DHT11 sensor!");
    }
    else {
      humidity = newHumidity;
      uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(humidity).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_HUM, packetIdPub2);
      Serial.printf("\nCurrent humidity value: %.2f\n", humidity);
    }
    // Read Gas Level
    int newGas = analogRead(Gas_analog);
    // if gas level read failed, don't change 'gas' variable value 
    if (isnan(newGas)) {
      Serial.println("Failed to read from MQ-2 sensor!");
    }
    else {
      gas = newGas;
      uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_GAS, 1, true, String(gas).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_GAS, packetIdPub3);
      Serial.printf("\nCurrent gas/smoke level value: %i\n", gas);
    }
    // Read Pressure
    int newPressure = bmp.readPressure() / 100;
    // if pressure level read failed, don't change 'pressure' variable value 
    if (isnan(newPressure)) {
      Serial.println("Failed to read from BMP280 sensor!");
    }
    else {
      pressure = newPressure;
      uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_PRES, 1, true, String(pressure).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_PRES, packetIdPub4);
      Serial.printf("\nCurrent pressure value: %.2f\n", pressure);
    }
    // Read Altitude
    int newAltitude = bmp.readAltitude(1013.25);
    // if altitude read failed, don't change 'altitude' variable value 
    if (isnan(newAltitude)) {
      Serial.println("Failed to read from BMP280 sensor!");
    }
    else {
      altitude = newAltitude;
      uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_ALT, 1, true, String(altitude).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId %i\n", MQTT_PUB_ALT, packetIdPub5);
      Serial.printf("\nCurrent altitude value: %i\n", altitude);
    }
  }
}
