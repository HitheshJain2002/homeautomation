#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h>

// === RFID Pins ===
#define SS_PIN 18
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// === Wi-Fi Credentials ===
const char* ssid = "Jinkalpa";
const char* password = "hithesh@1234";

// === MQTT Broker ===
const char* mqtt_server = "192.168.1.11";
WiFiClient espClient;
PubSubClient client(espClient);

// === User Structure ===
struct User {
  byte uid[7];
  byte uidSize;
  String name;
  String idString;
  bool isInside;
};

// === User List ===
User users[] = {
  {{0x04, 0x65, 0x39, 0x22, 0x4B, 0x67, 0x80}, 7, "Tilak_Shetty",  "zOEVrPeGUXONnd1SXXXz9tbZrKQ2", false},
  {{0xFC, 0x01, 0x96, 0x81},                   4, "Hithesh_Jain",  "cZS8rrcQ3hQgPRlgcrdZWlajQp62", false},
  {{0x92, 0xE6, 0x40, 0x1D},                   4, "Gaurav_BS",     "kes9iPrUF5OdyJ7BYfJCfVJGrLJ2", false}
};
const int userCount = sizeof(users) / sizeof(users[0]);

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_RFID")) {
      Serial.println(" connected");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

bool matchUID(byte* uid, byte size, const byte* ref, byte refSize) {
  if (size != refSize) return false;
  for (byte i = 0; i < size; i++) {
    if (uid[i] != ref[i]) return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  SPI.begin(5, 19, 23, 18); // SCK, MISO, MOSI, SS
  rfid.PCD_Init();

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  Serial.println("Tap your card to toggle entry/exit...");
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(100);
    return;
  }

  Serial.println("Card Detected!");
  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  bool userFound = false;

  for (int i = 0; i < userCount; i++) {
  if (matchUID(rfid.uid.uidByte, rfid.uid.size, users[i].uid, users[i].uidSize)) {
    userFound = true;
    users[i].isInside = !users[i].isInside;
    String action = users[i].isInside ? "Home" : "Vacation";

    String greeting = users[i].isInside ? "Welcome " + users[i].name : "Bye " + users[i].name;
    Serial.println(greeting);

    // Publish greeting to separate topic
    client.publish("rfid/greeting", greeting.c_str());

    // Create and publish JSON status
    String message = "{ \"uid\": \"" + users[i].idString + "\", \"action\": \"" + action + "\", \"name\": \"" + users[i].name + "\" }";

    Serial.println("Publishing to MQTT:");
    Serial.println("  Topic: rfid/status");
    Serial.println("  Message: " + message);

    client.publish("rfid/status", message.c_str());
    break;
  }
}

  if (!userFound) {
    Serial.println("Unknown card scanned.");
  }

  // Wait until card is removed
  while (rfid.PICC_IsNewCardPresent() || rfid.PICC_ReadCardSerial()) {
    delay(100);
  }

  rfid.PICC_HaltA(); 
  rfid.PCD_StopCrypto1();
  delay(5000);  // Avoid double scan
}
