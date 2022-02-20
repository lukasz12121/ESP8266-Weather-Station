#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <AsyncMqttClient.h>
#include "index.h"
#include "logout.h"

//wifi authentication
#define WIFI_SSID "iPhone"
#define WIFI_PASSWORD "qwerty123"

//http server authentication
#define HTTP_USERNAME "admin"
#define HTTP_PASSWORD "admin"

//mqtt server ip address and port
#define MQTT_HOST IPAddress(172,20,10,4)
//#define MQTT_HOST IPAddress(192,168,1,16)
#define MQTT_PORT 1883

//published topics
#define MQTT_PUB_TEMP "esp/dht/temperature"
#define MQTT_PUB_HUM "esp/dht/humidity"
#define MQTT_PUB_PRES "esp/bmp/pressure"
#define MQTT_PUB_GAS "esp/mq2/gas"
#define MQTT_PUB_ALT "esp/bmp/altitude"

//subscribed topics
#define MQTT_SUB_REL "esp/relay/lamp"

//DHT11 definitions
#define DHTPIN 14
#define DHTTYPE DHT11

//relay module pin definition
#define RELAY 13

//global sensor readings variables
float temperature = 0.0;
float humidity = 0.0;
int gas = 0;
float pressure = 0.0;
int altitude = 0;

//mq-2 sensor pin
int gas_analog = A0;   

//variables for sensor reading interal purposes
unsigned long previousMillis = 0;
unsigned long interval = 10000;

//dht object initialization
DHT dht(DHTPIN, DHTTYPE);

//bmp object initialization
Adafruit_BMP280 bmp;

//http server initialization
AsyncWebServer server(80);

//mqtt client initialization
AsyncMqttClient mqttClient;

//wifi handlers declaration
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

//relay state function for html switch purposes
String relayState(){
  if(digitalRead(RELAY)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}


//function for changing values of html placeholders
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
  else if(var == "BUTTON"){
    String button ="";
    String relayStateValue = relayState();
    button= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + relayStateValue + "><span class=\"slider\"></span></label></p>";
    return button;
  }
  return String();
}

//function to wifi connection establishment
void connectToWifi() {
  Serial.printf("\nConnecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

//function to mqtt broker connection establishment
void connectToMqtt() {
  Serial.printf("\nConnecting to MQTT...");
  mqttClient.connect();
}

//registration of function to handle wifi connection event
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.printf("\nConnected to Wi-Fi.");
  Serial.println(WiFi.localIP());
  connectToMqtt();
}

//registration of function to handle wifi disconnection event
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.printf("\nDisconnected from Wi-Fi.");
  connectToWifi();
}

//registration of function to handle mqqt broker connection event
void onMqttConnect(bool sessionPresent) {
  Serial.printf("\nConnected to MQTT.");
  Serial.printf("\nSession present: %s", sessionPresent ? "true" : "false");
  uint16_t packetIdSub1  = mqttClient.subscribe(MQTT_SUB_REL, 1);
  Serial.printf("\nSubscribing on topic %s, packetId: %i\n", MQTT_PUB_PRES, packetIdSub1);
}

//registration of function to handle mqqt broker disconnection event
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("\nDisconnected from MQTT.");
  if (WiFi.isConnected()) {
    connectToMqtt();
  }
}

//registration of function to handle successful topic publishing event
void onMqttPublish(uint16_t packetId) {
  Serial.printf("\nPublish acknowledged - packetId: %i\n", packetId);
}

//registration of function to handle receiving message from mqtt broker event
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.printf("\nMessage subscribed- topic: %s, payload: %s\n", topic, payload);
  char newPayload[len+1];
  newPayload[len] = '\0';
  strncpy(newPayload, payload, len);
  Serial.printf("\nNew payload: %s\n", newPayload);
  if((String(topic) == MQTT_SUB_REL)){
    if(String(newPayload) =="OFF"){
      digitalWrite(RELAY, LOW);
    }
    else{
      digitalWrite(RELAY, HIGH);
    }
  }
}

void setup(){
  Serial.begin(9600); 

  //relay initialization
  pinMode(RELAY, OUTPUT);

  //DHT sensor initialization
  dht.begin();

  //wifi event handlers initialiation 
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  delay(500);

  //BMP sensor initialization
  if(!bmp.begin()){
    Serial.printf("\nCould not find BMP280");
    while(1);
  }

  //mqtt connection event handling
  mqttClient.onConnect(onMqttConnect);
  //mqtt disconnection event handling
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqtt message receiving event handling
  mqttClient.onMessage(onMqttMessage);
  //mqtt publishing event handling
  mqttClient.onPublish(onMqttPublish);
  //mqtt server connection initialization
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  //wifi connect function call
  connectToWifi();

  // Print ESP8266 Local IP Address for HTTP server purposes
  Serial.println(WiFi.localIP());

  //main page upload request
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(HTTP_USERNAME, HTTP_PASSWORD))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
    Serial.printf("\nRequest on URL / sent successfully");
  });
  //logout from web page request
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
    Serial.printf("\nRequest on URL /logout sent successfully");
  });
  //logout page upload request
  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", logout_html, processor);
    Serial.printf("\nRequest on URL /logged-out sent successfully");
  });
  //relay module state update request
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputValue;

    if (request->hasParam("state")) {
      inputValue = request->getParam("state")->value();
      Serial.println(inputValue);
      digitalWrite(RELAY, inputValue.toInt());
    }
    else {
      Serial.println("No value provided");
    }
    request->send(200, "text/plain", "OK");
    Serial.printf("\nRequest on URL /update sent successfully");
  });
  //temperature value sending to web page request
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperature).c_str());
    Serial.printf("\nRequest on URL /temperature sent successfully");
  });
  //humidity value sending to web page request
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humidity).c_str());
    Serial.printf("\nRequest on URL /humidity sent successfully");
  });
  //gas/smoke level value sending to web page request
  server.on("/gas_level", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(gas).c_str());
    Serial.printf("\nRequest on URL /gas_level sent successfully");
  });
  //pressure values sending to web page request
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(pressure).c_str());
    Serial.printf("\nRequest on URL /pressure sent successfully");
  });
  //altitude value sending to web page request
  server.on("/altitude", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(altitude).c_str());
    Serial.printf("\nRequest on URL /altitude sent successfully");
  });
  
 //http server initialization
 server.begin();

}
 
void loop(){
  //setting logic for sensor reading interval
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    //read temperature
    float newTemperature = dht.readTemperature();
    // if temperature read failed, don't change 'temperature' variable value
    if (isnan(newTemperature)) {
      Serial.println("Failed to read from DHT11 sensor!");
    }
    else {
      temperature = newTemperature;
      //publishing temperature value
      uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temperature).c_str());
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_TEMP, packetIdPub1);
      Serial.printf("\nCurrent temperature value: %.2f\n", temperature);                          
    }
    //read Humidity
    float newHumidity = dht.readHumidity();
    //if humidity read failed, don't change 'humidity' variable value 
    if (isnan(newHumidity)) {
      Serial.println("Failed to read from DHT11 sensor!");
    }
    else {
      humidity = newHumidity;
      //publishing humidity value
      uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(humidity).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_HUM, packetIdPub2);
      Serial.printf("\nCurrent humidity value: %.2f\n", humidity);
    }
    //read Gas Level
    int newGas = analogRead(gas_analog);
    //if gas level read failed, don't change 'gas' variable value 
    if (isnan(newGas)) {
      Serial.println("Failed to read from MQ-2 sensor!");
    }
    else {
      gas = newGas;
      //publishing gas/smoke level value
      uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_GAS, 1, true, String(gas).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_GAS, packetIdPub3);
      Serial.printf("\nCurrent gas/smoke level value: %i\n", gas);
    }
    //read Pressure
    int newPressure = bmp.readPressure() / 100;
    // if pressure level read failed, don't change 'pressure' variable value 
    if (isnan(newPressure)) {
      Serial.println("Failed to read from BMP280 sensor!");
    }
    else {
      pressure = newPressure;
      //publishing pressure level value
      uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_PRES, 1, true, String(pressure).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i\n", MQTT_PUB_PRES, packetIdPub4);
      Serial.printf("\nCurrent pressure value: %.2f\n", pressure);
    }
    //read Altitude
    int newAltitude = bmp.readAltitude(1013.25);
    //if altitude read failed, don't change 'altitude' variable value 
    if (isnan(newAltitude)) {
      Serial.println("Failed to read from BMP280 sensor!");
    }
    else {
      altitude = newAltitude;
      //publishing altitude level value
      uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_ALT, 1, true, String(altitude).c_str()); 
      Serial.printf("\nPublishing on topic %s at QoS 1, packetId %i\n", MQTT_PUB_ALT, packetIdPub5);
      Serial.printf("\nCurrent altitude value: %i\n", altitude);
    }
  }
}
