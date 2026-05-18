#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// --- OLED Settings ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pin Definitions ---
#define BUZZER_PIN 14      // D5
#define ALARM_RESET_PIN 12 // D6
#define GATE_RESET_PIN 13  // D7
#define SERVO_PIN 2        // D4

// --- Servo Positions ---
#define GATE_OPEN 0
#define GATE_CLOSED 90

Servo entryGate;
String buffer = "";
bool alarmTriggered = false;
bool gateClosed = false;

void process(String data);
void updateOLED(String rawData, String statusStr);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize Servo
  entryGate.attach(SERVO_PIN);
  entryGate.write(GATE_OPEN); // Start with gate open

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(ALARM_RESET_PIN, INPUT_PULLUP);
  pinMode(GATE_RESET_PIN, INPUT_PULLUP);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("FOREST ENTRY SYSTEM");
  display.println("Gate: OPEN");
  display.display();
  
  Serial.println("SYSTEM READY - GATE OPEN");
}

void loop() {
  // 1. Handle Alarm Reset (Buzzer)
  if (alarmTriggered && digitalRead(ALARM_RESET_PIN) == LOW) {
    alarmTriggered = false;
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Buzzer Silenced");
    updateOLED("N/A", "BUZZER OFF");
    delay(200); 
  }

  // 2. Handle Gate Reset (Open Gate)
  if (gateClosed && digitalRead(GATE_RESET_PIN) == LOW) {
    gateClosed = false;
    entryGate.write(GATE_OPEN);
    Serial.println("Gate Opening...");
    updateOLED("N/A", "GATE OPENED");
    delay(200);
  }

  // 3. Handle incoming LoRa data
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      buffer.trim();
      if (buffer.length() > 0) process(buffer);
      buffer = "";
    } else {
      buffer += c;
    }
  }
}

void process(String data) {
  Serial.println("RX: " + data);
  String displayStatus = "";

  if (data.indexOf("WILD") >= 0) {
    displayStatus = "WILD ALERT!";
    
    // Trigger Buzzer
    if (!alarmTriggered) {
      alarmTriggered = true;
      digitalWrite(BUZZER_PIN, HIGH);
    }

    // Close Gate
    if (!gateClosed) {
      gateClosed = true;
      entryGate.write(GATE_CLOSED);
      Serial.println("ALERT: Closing Gate!");
    }
  }

  if (data.indexOf("NORMAL") >= 0) {
    displayStatus = "SYSTEM NORMAL";
  }

  updateOLED(data, displayStatus);
}

void updateOLED(String rawData, String statusStr) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("FOREST GATE CONTROL");
  display.println("--------------------");
  display.print("Status: "); display.println(statusStr);
  display.print("Gate: "); display.println(gateClosed ? "CLOSED" : "OPEN");
  
  if (alarmTriggered) display.println("! ALARM ACTIVE !");
  
  display.display();
}