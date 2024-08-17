/*
This sketch is to collect analoge reading and send to Node-red through MQTT Broker, also in this sketch wifimanger
is used to manage wifi connection, a rest button to reset wifi anc connect to a new one if needed, this makes it easy to 
join diferent wifi connection with out having to flash and harcode each time.
all calculations will be done on Node-red with this sketch.
*/

#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>

//Declaring PINS
#define RESET_PIN 0  // Pin for the reset button
#define relay1 14 //pin D14 on esp32
#define relay2 12 //pin D12 on esp32
#define analogPin 36 //analog pin

//Variables
int analogValue;



SimpleTimer Timer;

// MQTT Server
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883; // Replace with the actual port if different
const char* mqtt_user = "admin";
const char* mqtt_password = "idontknow"; 
WiFiClient espclient;
PubSubClient client(espclient);

WiFiManager wifiManager;

void setup_wifi() {
  wifiManager.setConfigPortalTimeout(30);
  // AutoConnect with timeout
  if(!wifiManager.autoConnect("ESP32-Smart-UPS")){
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MQTT Server :");
  Serial.print(mqtt_server);
}

void callback(char* topic, byte* payload, unsigned int length) {   //callback includes topic and payload ( from which (topic) the payload is comming)
// Shutdown / bootup comand
  for (int i=0;i<length;i++) {
    if (strcmp(topic,"esp32/battery_guage/inTopic")==0){
      char receivedChar = (char)payload[i];
      Serial.print(receivedChar);
      if (receivedChar == '1'){           
        digitalWrite(relay1, HIGH);
        client.publish("esp32/battery_guage/relaystatus", "Shutting Down Servers.....");
        delay(2000);
        digitalWrite(relay1, LOW);
        delay(1000);
        client.publish("esp32/battery_guage/relaystatus", "Shutdown Completed!");
      
      }else if  (receivedChar == '0'){           
        digitalWrite(relay1, HIGH);
        client.publish("esp32/battery_guage/relaystatus", "Booting Up Servers.....");
        delay(2000);
        digitalWrite(relay1, LOW);
        delay(1000);
        client.publish("esp32/battery_guage/relaystatus", "Booting Completed!");
      }
    }
 
   Serial.println();
  }
}



void reconnectMqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT Server1 connection...");
    //check if MQTT is with user id & pass or not, then connect 
    if(client.connect("espclient") || client.connect("espclient", mqtt_user, mqtt_password)){
      Serial.println("Server connected!");
      client.publish("esp32/outTopic", "server connected");
      client.subscribe("esp32/battery_guage/inTopic"); //topic=Demo
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(relay1,OUTPUT);
  pinMode(relay2,OUTPUT);
  
  //analog read pin
  pinMode(analogPin,INPUT_PULLUP);

  pinMode(RESET_PIN, INPUT_PULLUP); // Set the reset button pin as input with internal pull-up
 
  //Timer interval for sending sensor reading
  Timer.setInterval(10000);

  // Check if the reset button is pressed, this is not a mistake repetition, having this reset
  //setings here helps to break the loop incase code gets stock on MQTT reconnect loop.
  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println("Reset button pressed. Starting configuration portal...");
    delay(2000);
    wifiManager.resetSettings();
    wifiManager.startConfigPortal("ESP32_SMART_UPS");
  }

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
}

void loop() {
  if (!client.connected()) {
    reconnectMqtt();
  }
  client.loop();

  // Check if the reset button is pressed during normal operation
  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println("Reset button pressed. Starting configuration portal...");
    delay(2000);
    wifiManager.resetSettings();
    wifiManager.startConfigPortal("ESP32-Smart-UPS");
  }

  if (Timer.isReady()) {      // Check is ready a first timer
    Serial.println("Battery Voltage:");
    client.publish("deviceState", "running");
    client.publish("deviceStateRemote", "running"); 
    sensorReading ();                         
    Timer.reset();  // Reset a second timer
    } 
  
}

void sensorReading (){
  //Analog reading;
  analogValue = analogRead(analogPin);

  //printing result to serial monitor on IDE
  Serial.print("Analog: ");
  Serial.println(analogValue);

  //for average analog reading calculation
  //if you wish to publish data as string with a topic uncomment the code below  
  client.publish("esp32/battery_guage/analogSensorAvarage", String(analogValue).c_str());

  // Stream& output;
  //Sending sensor reading as JSON instead of plain string via a topic too
  JsonDocument doc;
  JsonObject sensor = doc["sensor"].to<JsonObject>();
  sensor["smartUps"] = "Smart-UPS-Project"; 
  sensor["analogValue"] = analogValue;

  doc.shrinkToFit();  // optional
  char output[256];
  serializeJson(doc, output);
  client.publish("EspSensor", output);
  
}
