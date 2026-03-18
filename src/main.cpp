#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ModbusMaster.h>

#define PIN_IN1 8
#define PIN_IN2 9
#define PIN_RX  20
#define PIN_TX  21
#define PIN_DE_RE 10 // Optional DE/RE pin for RS485

AsyncWebServer server(80);
Preferences preferences;
ModbusMaster node;

// System State
enum MotorState { IDLE, RUNNING_RIGHT, RUNNING_LEFT, MOTOR_ERROR };
MotorState currentState = IDLE;
unsigned long lastStateSent = 0;

// Configuration variables
int speedFinal = 1000;
int delta = 100;
int gap = 200; // ms
int modbusAddress = 1;
int modbusBaudrate = 9600;

// Dynamic control
unsigned long lastGapTime = 0;
int currentSpeed = 0;
bool speedGoingUp = true;

// Web Control Overrides
bool webGiroDerecha = false;
bool webGiroIzquierda = false;
bool webStop = false;

// Dummy registers for Modbus (To be replaced with real driver registers)
#define REG_SPEED_CTRL 0x01
#define REG_DIR_CTRL 0x02

void preTransmission() { digitalWrite(PIN_DE_RE, 1); }
void postTransmission() { digitalWrite(PIN_DE_RE, 0); }

void loadPreferences() {
  preferences.begin("motor", false);
  speedFinal = preferences.getInt("speedFinal", 1000);
  delta = preferences.getInt("delta", 100);
  gap = preferences.getInt("gap", 200);
  if (gap < 100) gap = 100;
  if (gap > 1000) gap = 1000;
  modbusAddress = preferences.getInt("mbAddress", 1);
  modbusBaudrate = preferences.getInt("mbBaudrate", 9600);
}

void savePreferences() {
  preferences.putInt("speedFinal", speedFinal);
  preferences.putInt("delta", delta);
  preferences.putInt("gap", gap);
  preferences.putInt("mbAddress", modbusAddress);
  preferences.putInt("mbBaudrate", modbusBaudrate);
}

void setupModbus() {
  Serial1.begin(modbusBaudrate, SERIAL_8N1, PIN_RX, PIN_TX);
  node.begin(modbusAddress, Serial1);
  pinMode(PIN_DE_RE, OUTPUT);
  digitalWrite(PIN_DE_RE, 0);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-MotorControl", "12345678");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void handleModbusControl() {
  if (currentState == IDLE || currentState == MOTOR_ERROR) {
    // Send stop command
    node.writeSingleRegister(REG_SPEED_CTRL, 0);
    return;
  }

  // Calculate dynamic speed
  if (millis() - lastGapTime >= gap) {
    lastGapTime = millis();
    int minSpeed = speedFinal - delta;
    int maxSpeed = speedFinal + delta;
    if (minSpeed < 0) minSpeed = 0;

    if (speedGoingUp) {
      currentSpeed += (delta / 2); // Sample step
      if (currentSpeed >= maxSpeed) {
        currentSpeed = maxSpeed;
        speedGoingUp = false;
      }
    } else {
      currentSpeed -= (delta / 2);
      if (currentSpeed <= minSpeed) {
        currentSpeed = minSpeed;
        speedGoingUp = true;
      }
    }
    
    // Write speed
    node.writeSingleRegister(REG_SPEED_CTRL, currentSpeed);
    // Write direction
    uint16_t dirReg = (currentState == RUNNING_RIGHT) ? 1 : 2;
    node.writeSingleRegister(REG_DIR_CTRL, dirReg);
  }
}

void setupRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/app.js", "application/javascript");
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<256> doc;
    doc["state"] = currentState == IDLE ? "Idle" : (currentState == RUNNING_RIGHT ? "Running Right" : (currentState == RUNNING_LEFT ? "Running Left" : "Error"));
    doc["speed"] = currentSpeed;
    doc["speedFinal"] = speedFinal;
    doc["delta"] = delta;
    doc["gap"] = gap;
    doc["mbAddress"] = modbusAddress;
    doc["mbBaudrate"] = modbusBaudrate;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("speedFinal", true) && request->hasParam("delta", true) && request->hasParam("gap", true)) {
      speedFinal = request->getParam("speedFinal", true)->value().toInt();
      delta = request->getParam("delta", true)->value().toInt();
      gap = request->getParam("gap", true)->value().toInt();
      if(gap < 100) gap = 100;
      if(gap > 1000) gap = 1000;
      savePreferences();
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Action buttons via POST (pushbutton logic from Web UI: start and stop actions)
  server.on("/api/action", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("cmd", true)) {
      String cmd = request->getParam("cmd", true)->value();
      if(cmd == "right_start") { webGiroDerecha = true; }
      else if(cmd == "right_stop") { webGiroDerecha = false; }
      else if(cmd == "left_start") { webGiroIzquierda = true; }
      else if(cmd == "left_stop") { webGiroIzquierda = false; }
      else if(cmd == "estop") { webStop = true; }
      else if(cmd == "estop_release") { webStop = false; }
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_IN1, INPUT_PULLUP);
  pinMode(PIN_IN2, INPUT_PULLUP);

  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  loadPreferences();
  setupModbus();
  setupWiFi();
  setupRoutes();
}

void loop() {
  bool in1 = digitalRead(PIN_IN1) == LOW; // Assuming active low
  bool in2 = digitalRead(PIN_IN2) == LOW;

  // Determine intended state based on physical switches AND web inputs
  MotorState targetState = IDLE;

  if (webStop) {
    targetState = IDLE;
  } else if ((in1 && in2) || (webGiroDerecha && webGiroIzquierda)) {
    targetState = IDLE;
  } else if (in1 || webGiroDerecha) {
    targetState = RUNNING_RIGHT;
  } else if (in2 || webGiroIzquierda) {
    targetState = RUNNING_LEFT;
  }

  if(targetState != currentState) {
    currentState = targetState;
    if(currentState == IDLE) { currentSpeed = 0; }
    else { currentSpeed = speedFinal; } // Start at target immediately, will dither
    lastGapTime = millis();
  }

  handleModbusControl();
  
  delay(10); // Small delay to yield to WiFi/Web tasks
}
