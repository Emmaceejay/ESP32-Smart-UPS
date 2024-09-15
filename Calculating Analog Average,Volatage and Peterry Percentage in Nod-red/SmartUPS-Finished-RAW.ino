/*
This sketch collects analog reading and sends it to Node-red through MQTT Broker, also in this sketch wifimanger
is used to manage the wifi connection, and a reset button to reset the wifi and connect to a new one if needed, this makes it easy to 
join different wifi connection without having to flash and hardcode each time.
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

void callback(char* topic, byte* payload, unsigned int length) {   //callback includes topic and payload ( from which (topic) the payload is coming)
//Shutdown/bootup command (power button simulation
  for (int i=0;i<length;i++) {
    if (strcmp(topic,"esp32/battery_guage/inTopic")==0){
      char receivedChar = (char)payload[i];
      Serial.print(receivedChar);
      if (receivedChar == '1'){           
        digitalWrite(relay1, HIGH);
        client.publish("esp32/battery_guage/relaystatus", "Shutting Down Servers.....");
        delay(2000);
        digitalWrite(relay1, LOW);
        delay(10000);
        client.publish("esp32/battery_guage/relaystatus", "Shutdown Completed!");
      
      }else if  (receivedChar == '0'){           
        digitalWrite(relay1, HIGH);
        client.publish("esp32/battery_guage/relaystatus", "Booting Up Servers.....");
        delay(2000);
        digitalWrite(relay1, LOW);
        delay(10000);
        client.publish("esp32/battery_guage/relaystatus", "Booting Completed!");
      }
    }
 
   Serial.println();
  }
}



void reconnectMqtt() {
  while (!client.connected()) {

     // Check if the reset button is pressed, this is not a mistake repetition, having this reset
   //settings here help to break the loop in case the code gets stuck on the MQTT reconnect loop.
    if (digitalRead(RESET_PIN) == LOW) {
      Serial.println("Reset button pressed. Starting configuration portal...");
      delay(2000);
      wifiManager.resetSettings();
      wifiManager.startConfigPortal("ESP32_SMART_UPS");
    }
    
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
  pinMode(analogPin,INPUT_PULLUP); //analog read pin
  pinMode(RESET_PIN, INPUT_PULLUP); // Set the reset button pin as input with internal pull-up
  Timer.setInterval(10000); //Timer interval for sending sensor reading
  
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

  doc.shrinkToFit();  // optional#include <WiFiManager.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>

// Pin Definitions
#define RESET_PIN 0
#define relay1 14
#define relay2 13
const int analogPin = 36;
const int numReadings = 5;  // Reduced for faster averaging

// Variables for Averaging
int readings[numReadings];
int readIndex = 0;
int total = 0;
int average = 0;

// Variables for Low-Pass Filter
float filteredValue = 0.0;
const float alpha = 0.1;  // Faster response

// Voltage Divider Scale Factor
const float scaleFactor = 3.86;

// Variables for calculating voltage and percentage
float voltage;
float batPercentage;
float batteryPercentCalc;

// Battery Voltage Range
const float maxVoltage = 12.3;
const float minVoltage = 10.9;

// Timer variable declarations
SimpleTimer Timer;
SimpleTimer sensorTimer;
SimpleTimer mosfetTimer;

// MQTT Parameters
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
WiFiClient espclient;
PubSubClient client(espclient);

WiFiManager wifiManager;

void setup_wifi() {
  wifiManager.setConfigPortalTimeout(30);
  if(!wifiManager.autoConnect("ESP32-Smart-UPS")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mosfetSwitch() {
  if (mosfetTimer.isReady()) {
    digitalWrite(relay2, HIGH);  // Turn on MOSFET
    delay(60000);  // Keep it on for 60 seconds
    digitalWrite(relay2, LOW);   // Turn off MOSFET
    mosfetTimer.reset();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {   
  for (int i = 0; i < length; i++) {
    if (strcmp(topic, "esp32/battery_guage/inTopic") == 0) {
      char receivedChar = (char)payload[i];
      Serial.print(receivedChar);
      if (receivedChar == '1') {           
        digitalWrite(relay1, HIGH);
        client.publish("esp32/battery_guage/relaystatus", "Shutting Down Servers.....");
        delay(2000);
        digitalWrite(relay1, LOW);
        delay(9000);
        client.publish("esp32/battery_guage/relaystatus", "Shutdown Completed!");
      } else if (receivedChar == '0') {           
        digitalWrite(relay1, HIGH);
        client.publish("esp32/battery_guage/relaystatus", "Booting Up Servers.....");
        delay(2000);
        digitalWrite(relay1, LOW);
        delay(9000);
        client.publish("esp32/battery_guage/relaystatus", "Booting Completed!");
      }
    }
    Serial.println();
  }
}

void reconnectMqtt() {
  while (!client.connected()) {
    if (digitalRead(RESET_PIN) == LOW) {
      Serial.println("Reset button pressed. Starting configuration portal...");
      delay(2000);
      wifiManager.resetSettings();
      wifiManager.startConfigPortal("ESP32_SMART_UPS");
    }

    Serial.print("Attempting MQTT Server connection...");
    if (client.connect("espclient")) {
      Serial.println("Server connected!");
      client.publish("esp32/outTopic", "Server connected");
      client.subscribe("esp32/battery_guage/inTopic");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Try again in 5 seconds");
      delay(5000);
    }
  }
}

void processADCReadings() {
  // Update readings array and calculate filtered value
  total = total - readings[readIndex];
  int newReading = analogRead(analogPin);
  total = total + newReading;
  readings[readIndex] = newReading;
  
  readIndex = (readIndex + 1) % numReadings;
  average = total / numReadings;
  
  // Apply low-pass filter
  filteredValue = (alpha * average) + ((1.0 - alpha) * filteredValue);
  voltage = ((filteredValue * 3.3) / 4095) * scaleFactor;
  batteryPercentCalc = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;
  batPercentage = constrain(batteryPercentCalc, 0.0, 100.0);
  
  // Print the values for debugging
  Serial.print("Voltage: ");
  Serial.println(voltage, 2);
  Serial.print("Battery Percentage: ");
  Serial.println(batPercentage, 1);
  Serial.print("Raw Value: ");
  Serial.println(newReading);
  Serial.print("Averaged Value: ");
  Serial.println(average);
  Serial.print("Filtered Value: ");
  Serial.println(filteredValue);
}

void setup() {
  Serial.begin(115200);

  // Pin modes
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay2, LOW);  // Initially LOW for battery reading

  pinMode(RESET_PIN, INPUT_PULLUP);

  // Timer intervals
  Timer.setInterval(10000); // 10 seconds for faster results
  sensorTimer.setInterval(1000); // Faster sampling and filtering
  mosfetTimer.setInterval(60000);  // 60 seconds for MOSFET

  analogSetAttenuation(ADC_11db);
  analogSetWidth(12);

  // Initialize the readings array with 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
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

  if (Timer.isReady()) {
    client.publish("deviceState", "running");
    Timer.reset();
  }

  // Take initial battery reading when MOSFET is LOW
  if (digitalRead(relay2) == LOW && sensorTimer.isReady()) {
    processADCReadings();
    
    // Publish values to MQTT
    client.publish("esp32/battery_guage/analogSensorAvarage", String(filteredValue).c_str());
    client.publish("esp32/battery_guage/voltReading2", String(voltage).c_str());
    client.publish("esp32/battery_guage/percent2", String(batPercentage).c_str());

    // Construct JSON object
    StaticJsonDocument<256> doc;
    JsonObject sensor = doc.createNestedObject("sensor");
    sensor["smartUps"] = "Smart-UPS-Project";
    sensor["analogValue"] = filteredValue;
    sensor["voltage"] = voltage;
    sensor["batPercentage"] = batPercentage;
    char output[256];
    serializeJson(doc, output);
    client.publish("EspSensor", output);

    sensorTimer.reset();
  }

  // MOSFET control after initial battery reading
  mosfetSwitch();
}

  char output[256];
  serializeJson(doc, output);
  client.publish("EspSensor", output);
  
}
