
#include <Arduino.h>

// ---------------- UART ----------------
HardwareSerial PiSerial(2);
HardwareSerial LoRa(1);
HardwareSerial GPS(0);

// ---------------- DATA ----------------
String lastDetect = "";
String gpsData = "";

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  PiSerial.begin(115200, SERIAL_8N1, 26, 27); //CONNECTED PINS (RASP-Tx GPIO14 to D4 & RASP-Rx GPIO15 to RX2)
  GPS.begin(9600, SERIAL_8N1, 18, 19);  
  LoRa.begin(115200, SERIAL_8N1, 4, 5);//CONNECTED PINS (lora rx to D18 & lora tx to tx2)

  Serial.println("ESP32 READY");
}

// ---------------- LOOP ----------------
void loop() {

  // -------- PI INPUT --------
  if (PiSerial.available()) {
    String msg = PiSerial.readStringUntil('\n');
    msg.trim();

    if (msg.length() > 0) {
      lastDetect = msg;
      Serial.println("PI: " + msg);
    }
  }

  // -------- GPS INPUT --------
  if (GPS.available()) {
    gpsData = GPS.readStringUntil('\n');
  }

  // -------- BUILD + SEND --------
  String packet = buildPacket(lastDetect, gpsData);
  sendLoRa(packet);

  delay(3000);
}

// ---------------- PACKET ----------------
String buildPacket(String detect, String gps) {

  String timeStamp = String(millis() / 1000);

  String status = "NORMAL";
  String type = "None";
  String conf = "None";

  if (detect.startsWith("DETECT")) {
    int p1 = detect.indexOf('|');
    int p2 = detect.indexOf('|', p1 + 1);
    int p3 = detect.indexOf('|', p2 + 1);

    status = detect.substring(p1 + 1, p2);
    type   = detect.substring(p2 + 1, p3);
    conf   = detect.substring(p3 + 1);
  }

  return "TIME:" + timeStamp +
         " | STATUS:" + status +
         " | TYPE:" + type +
         " | CONF:" + conf +
         " | GPS:" + gps;
}

// ---------------- LORA ----------------
void sendLoRa(String data) {
  LoRa.println(data);
  Serial.println("LoRa TX: " + data);
}