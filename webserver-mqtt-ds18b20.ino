// Load Wi-Fi library
#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <TypeConversionFunctions.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>

// LED is PIN GPIO2 (D4)
#define LED D4

bool test = false;

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     // DHTPin 

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Holds the number of connected DS18B20 sensors
int numberOfDevices = -1;

// variable to hold device addresses
DeviceAddress thermometer;

/** SSID and password of Wifi */
const char* ssid     = "change"; //<<<<<<<<<<<<<<<
const char* password = "change"; //<<<<<<<<<<<<<<<<

/** Name of this Wifi-client */
const char* hostname     = "change"; //<<<<<<<<<<<<<<<

// MQTT broker credentials (set to NULL if not required)
const char* MQTT_username = "change"; //<<<<<<<<<<<<<<<
const char* MQTT_password = "change"; //<<<<<<<<<<<<<<<
// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "1.2.3.4"; //<<<<<<<<<<<<<<< example: 192.168.0.5
const int mqtt_port = 1883; //<<<<<<<<<<<<<<<
const char* mqtt_client_name = "change"; //<<<<<<<<<<<<<<<

// Set web server port number to 80
ESP8266WebServer server(80); //using ESP8266WebServer.h

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient pubSubClient(espClient);

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<header>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- jQuery -->
  <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.7.1/jquery.min.js" 
          integrity="sha512-v2CJ7UaYy4JwqLDIrZUI/4hqeoQieOmAZNXBeQyjo21dadnwR+8ZaIJVT8EE2iyI61OV8e6M8PP2/4hpQINQ/g==" 
          crossorigin="anonymous" referrerpolicy="no-referrer"></script>

  <!-- Bootstrap -->
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.1/font/bootstrap-icons.css">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.2/css/bootstrap.min.css" 
        integrity="sha512-b2QcS5SsA8tZodcDtGRELiGv5SaKSk1vDHDaQRda0htPYWZ6046lr3kJ5bAAQdpV2mmA/4v0wQF9MyU6/pDIAg==" 
        crossorigin="anonymous" referrerpolicy="no-referrer" />
  <script src="https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.2/js/bootstrap.min.js" 
          integrity="sha512-WW8/jxkELe2CAiE4LvQfwm1rajOS8PHasCCx+knHG0gBHt8EXxS6T6tJRTGuDQVnluuAvMxWF4j8SNFDKceLFg==" 
          crossorigin="anonymous" referrerpolicy="no-referrer"></script>

  <!-- momentJs -->
  <script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.29.4/moment.min.js" crossorigin="anonymous" referrerpolicy="no-referrer"></script>

  <script>
    document.addEventListener("DOMContentLoaded", function(event) { 
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == XMLHttpRequest.DONE && this.status == 200) {
          var tmp = JSON.parse(this.responseText);
          $("#ssid").val(tmp.WiFi.SSID);
          var statusText = "";
          if(tmp.WiFi.status == 0) { statusText = "idle"; }
          else if(tmp.WiFi.status == 1) { statusText = "no SSID available"; }
          else if(tmp.WiFi.status == 2) { statusText = "scan completed"; }
          else if(tmp.WiFi.status == 3) { statusText = "connected"; }
          else if(tmp.WiFi.status == 4) { statusText = "connect failed"; }
          else if(tmp.WiFi.status == 5) { statusText = "connection lost"; }
          else if(tmp.WiFi.status == 6) { statusText = "wrong password"; }
          else if(tmp.WiFi.status == 7) { statusText = "disconnected"; }
          $("#status").val(statusText);
          $("#hostname").val(tmp.WiFi.hostname);
          $("#localIP").val(tmp.WiFi.localIP);
          $("#webserverRoot").val(tmp.WebServer.root);
          if(tmp.led.status == "on") {
            $("#ledStatusSwitch").prop('checked', true);
          } else {
            $("#ledStatusSwitch").prop('checked', false);
          }
          if(tmp.testmode) {
            $("#testmodeSwitch").prop('checked', true);
          } else {
            $("#testmodeSwitch").prop('checked', false);
          }
          var uptimeTmp = moment.duration(tmp.uptime);
          $("#uptime").val(uptimeTmp.toISOString()+" ("+ uptimeTmp.locale("en").humanize()+")");
          $("#countOfDS18B20Sensors").val(tmp.sensors.DS18B20.count);
          for(var i = 0; i < tmp.sensors.DS18B20.count; i++) {
            var sensorData = tmp.sensors.DS18B20['sensor'+(i+1)];
            $("#temperaturSensorTableBody").append(`<tr><th scope="row">${(i+1)}</th><td>${sensorData.temperature} ${sensorData.unitOfMeasure}</td><td>tbd</td><td>${sensorData.deviceAddress}</td>`);
          }
        }
      };
      xhttp.open("GET", "/info", true);
      xhttp.send();
    });
    $(document).ready(function() {
      $("#ledStatusSwitch").click(function() {
        var xhttp = new XMLHttpRequest();
        xhttp.open("GET", "/toggleLedAndStatus", true);
        xhttp.send();
      });
      $("#testmodeSwitch").click(function() {
        var xhttp = new XMLHttpRequest();
        xhttp.open("GET", "/toggleTestmode", true);
        xhttp.send();
      });
    });
  </script>
</header>
<body>
  <nav class="navbar bg-body-tertiary">
    <div class="container-fluid">
      <a class="navbar-brand" href="#"><i class="bi bi-cpu-fill"></i> ESP8266</a>
    </div>
  </nav>
  <div class="container">
    <div class="mb-3 row">
      <label for="uptime" class="col-sm-2 col-form-label"><i class="bi bi-clock"></i> Uptime</label>
      <div class="col-sm-10">
        <input type="text" readonly class="form-control-plaintext" id="uptime" value="">
      </div>
    </div>
    <div class="mb-3 row">
      <label for="ssid" class="col-sm-2 col-form-label"><i class="bi bi-wifi"></i> SSID</label>
      <div class="col-sm-10">
        <input type="text" readonly class="form-control-plaintext" id="ssid" value="">
      </div>
    </div>
    <div class="mb-3 row">
      <label for="status" class="col-sm-2 col-form-label"><i class="bi bi-heart-pulse"></i> Status</label>
      <div class="col-sm-10">
        <input type="text" readonly class="form-control-plaintext" id="status" value="">
      </div>
    </div>
    <div class="mb-3 row">
      <label for="hostname" class="col-sm-2 col-form-label"><i class="bi bi-tag"></i> Hostname</label>
      <div class="col-sm-10">
        <input type="text" readonly class="form-control-plaintext" id="hostname" value="">
      </div>
    </div>
    <div class="mb-3 row">
      <label for="localIP" class="col-sm-2 col-form-label"><i class="bi bi-hdd-network"></i> Local IP</label>
      <div class="col-sm-10">
        <input type="text" readonly class="form-control-plaintext" id="localIP" value="">
      </div>
    </div>
    <div class="mb-3 row">
      <label for="webserverRoot" class="col-sm-2 col-form-label"><i class="bi bi-globe"></i> WebServer root</label>
      <div class="col-sm-10">
        <input type="text" readonly class="form-control-plaintext" id="webserverRoot" value="">
      </div>
    </div>
    <div class="mb-3 row">
      <label for="countOfDS18B20Sensors" class="col-sm-2 col-form-label"><i class="bi bi-thermometer-high"></i> DS18B20 Sensors</label>
      <div class="col-sm-10">
        <input type="text" readonly class="form-control-plaintext" id="countOfDS18B20Sensors" value="">
      </div>
    </div>
    <div class="mb-3 row">
      <div class="form-check form-switch col-sm-2" style="padding-left: 3.2em;">
        <input class="form-check-input" type="checkbox" role="switch" id="testmodeSwitch" checked>
        <label class="form-check-label" for="testmodeSwitch" id="testmodeSwitchLabel">Test Mode</label>
      </div>
    </div>
    <div class="mb-3 row">
      <div class="form-check form-switch col-sm-2" style="padding-left: 3.2em;">
        <input class="form-check-input" type="checkbox" role="switch" id="ledStatusSwitch" checked>
        <label class="form-check-label" for="ledStatusSwitch" id="ledStatusSwitchLabel">LED</label>
      </div>
    </div>
    <h5>Temperature Sensors</h5>
    <table class="table table-striped table-hover">
      <thead>
        <tr>
          <th scope="col">#</th>
          <th scope="col">ºC</th>
          <th scope="col">Label</th>
          <th scope="col">Address</th>
        </tr>
      </thead>
      <tbody id="temperaturSensorTableBody">
      </tbody>
    </table>
  </div>
</body>
</html>
)=====";

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

/** This functions reconnects your ESP8266 to your MQTT broker */
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (pubSubClient.connect(mqtt_client_name, MQTT_username, MQTT_password)) {
      Serial.println("connected");  
      // Subscribe or resubscribe to topics
      String tmp = String(mqtt_client_name) + "/led";
      pubSubClient.subscribe(tmp.c_str());
      tmp = String(mqtt_client_name) + "/testmode";
      pubSubClient.subscribe(tmp.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000); //todo: switch to using millis()
    }
  }
}

void toggleLed() {
  if(digitalRead(LED) == HIGH) {
    //Switch LED on
    digitalWrite(LED, LOW);
  } else {
    //Switch LED off
    digitalWrite(LED, HIGH);
  }
}

void switchLedOn() {
  //Switch LED on
  digitalWrite(LED, LOW);
}
void switchLedOff() {
  //Switch LED on
  digitalWrite(LED, HIGH);
}

void handleRoot() {
  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}

String listSensors() {
  toggleLed();
  sensors.requestTemperatures(); 
  String s = "";
  //Serial.println("Printing addresses and temperatures ...");
  for (int i = 0;  i < numberOfDevices;  i++) {
    s = s + "\"sensor"+(i+1)+"\" : {";
    s = s + "\"temperature\" : ";
    if(test) {
    s = s + (23+random(10, 20));
    } else {
    s = s + sensors.getTempCByIndex(i);
    }
    s = s + ",";
    s = s + "\"unitOfMeasure\" : \"ºC\",";
    if(test) {
      thermometer[0] = 177+i;
      thermometer[1] = 24+i;
      thermometer[2] = 93+i;
      thermometer[3] = 201+i;
      thermometer[4] = 233+i;
      thermometer[5] = 123+i;
      thermometer[6] = 43+i;
      thermometer[7] = 17+i;
    } else {
      sensors.getAddress(thermometer, i);
    }
    s = s + "\"deviceAddress\" : \""+uint8ArrayToHexString(thermometer, 8)+"\"";
    s = s + "}";
    if(i < numberOfDevices-1) {
      s = s +",";
    }
    //sensors.getAddress(thermometer, i);
    //printAddress(thermometer);
  }
  toggleLed();
  return s;
}

void handleInfo() {
  uint8_t ledState = digitalRead(LED);
  toggleLed();
  String s = "{ ";
  s = s + "\"WiFi\" : {";
  s = s +   "\"SSID\": \""+String(ssid)+"\",";
  s = s +   "\"status\": \""+WiFi.status()+"\",";
  s = s +   "\"statusCodes\": { ";
  s = s +     "\"idle\": 0,";
  s = s +     "\"noSsidAvailable\": 1,";
  s = s +     "\"scanCompleted\": 2,";
  s = s +     "\"connected\": 3,";
  s = s +     "\"connectFailed\": 4,";
  s = s +     "\"connectionLost\": 5,";
  s = s +     "\"wrongPassword\": 6,";
  s = s +     "\"disconnected\": 7 },";
  s = s +   "\"hostname\": \""+String(hostname)+"\",";
  s = s +   "\"localIP\": \""+WiFi.localIP().toString()+"\"";
  s = s + "},";
  s = s + "\"WebServer\" : {";
  s = s +   "\"root\": \"http://"+WiFi.localIP().toString()+":80/\"";
  s = s + "},";
  s = s + "\"uptime\" : "+millis()+",";
  s = s + "\"sensors\" : {";
  s = s +   "\"DS18B20\" : {";
  if(numberOfDevices > 0) {
  s = s +     "\"count\" : "+numberOfDevices+",";
  s = s + listSensors(); 
  } else {
    s = s +     "\"count\" : "+numberOfDevices;
  }
  s = s +   "}";
  s = s + "},";
  s = s + "\"led\" : {";
  if(ledState == HIGH) {
    s = s + "\"status\": \"off\"";
  } else {
    s = s + "\"status\": \"on\"";
  }
  s = s + "},";
  s = s + "\"testmode\" : ";
  if(test == true) {
    s = s + "true";
  } else {
    s = s + "false";
  } 
  s = s + "}";
  toggleLed();
  server.send(200, "application/json", s);
}

// List all DS18B20 (temperature) sensors connected to this ESP8266
void handleListSensors() {
  String s = listSensors();
  server.send(200, "application/json", s);
}

void ledStatus() {
  String s = "{";
  if(digitalRead(LED) == HIGH) {
    s = s + "\"led\": \"off\"";
  } else {
    s = s + "\"led\": \"on\"";
  }
  s = s + "}";
  server.send(200, "application/json", s);
}

void toggleLedAndStatus() {
  toggleLed();
  ledStatus();
}

void toggleTestmode() {
  if(test) {
    Serial.println("toggle test mode to false (off)");
    test = false;
    numberOfDevices = sensors.getDeviceCount();
  } else {
    Serial.println("toggle test mode to true (on)");
    test = true;
    numberOfDevices = 8;
  }
}

void publishTemperatures() {
  Serial.println("publish temperatures to \"room/temperature\"");

  sensors.requestTemperatures(); 
  for (int i = 0;  i < numberOfDevices;  i++) {
    toggleLed();

    if(test) {
      thermometer[0] = 177+i;
      thermometer[1] = 24+i;
      thermometer[2] = 93+i;
      thermometer[3] = 201+i;
      thermometer[4] = 233+i;
      thermometer[5] = 123+i;
      thermometer[6] = 43+i;
      thermometer[7] = 17+i;
    } else {
      sensors.getAddress(thermometer, i);
    }

    StaticJsonDocument<256> jsonDoc;
    jsonDoc["sensorType"] = "DS18B20";
    jsonDoc["address"] = uint8ArrayToHexString(thermometer, 8);
    jsonDoc["unitOfMeasure"] = "ºC";
    if(test) {
      jsonDoc["temperature"] = 15+random(10, 20);
    } else {
      jsonDoc["temperature"] = sensors.getTempCByIndex(i);
    }

    char out[265];
    int b = serializeJson(jsonDoc, out);
    pubSubClient.publish("room/temperature", out);
    Serial.println(out);
  }
  if(numberOfDevices == 0) {
    toggleLed();
    Serial.println("No temperature devices found.");
    toggleLed();
  }
}

void callback(String topic, byte* message, unsigned int length) {
  // Output the message;
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  String tmp = String(mqtt_client_name) + "/led";
  if(topic == tmp){
    if(messageTemp == "on"){
      switchLedOn();
    } else if(messageTemp == "off"){
      switchLedOff();
    }
  }
  tmp = String(mqtt_client_name) + "/testmode";
  if(topic == tmp) {
    /*if(messageTemp == "on"){
      test = true;
      numberOfDevices = 8;
    } else if(messageTemp == "off"){
      test = false;
      numberOfDevices = sensors.getDeviceCount();
    }*/
  }
}

void setup() {
  // Set the LED pin for output
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  
  delay(2000);
  Serial.begin(115200);  

  // Start the DS18B20 sensors
  sensors.begin();
  if(test) {
    numberOfDevices = 8;
  } else {
    numberOfDevices = sensors.getDeviceCount();
  }
  Serial.print("Count of found sensors: ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices");

  // Connect to WLAN
  Serial.print("Connecting WLAN with SSID ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  WiFi.hostname(hostname); // can only be set after Wifi.begin()
  // Output a dot every 500ms until the connect to WLAN is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  // Print local IP address
  Serial.println("WLAN connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the webserver
  server.begin();
  Serial.print("Webserver started. Listening at ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println(":80/");

  server.on("/", handleRoot);
  server.on("/info", handleInfo);
  server.on("/toggleLedAndStatus", toggleLedAndStatus);
  server.on("/toggleTestmode", toggleTestmode);
  server.on("/ledStatus", ledStatus);
  server.on("/listSensors", handleListSensors);

  //MQTT
  Serial.println("Connecting to MQTT server ...");
  pubSubClient.setServer(mqtt_server, mqtt_port);
  pubSubClient.setCallback(callback);
  reconnect();
}

void loop() {
  // Handle client requests
  server.handleClient();

  // Ensure that we are connected to the MQTT broker
  if (!pubSubClient.connected()) {
    reconnect();
  }
  if(!pubSubClient.loop()) { // todo: was ist der Zweck hiervon?
    pubSubClient.connect(mqtt_client_name, MQTT_username, MQTT_password);  
  }

  // Perform actions every 30 seconds
  now = millis();
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
    publishTemperatures();
  }
}
