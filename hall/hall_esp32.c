#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h>

// === RFID Pins ===
#define SS_PIN 18
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// === Buzzer Pin ===
#define BUZZER_PIN 15  // D15

// === Wi-Fi Credentials ===
const char* ssid = "Qwerty";
const char* password = "qwerty@123";

// === MQTT Broker ===
const char* mqtt_server = "172.16.10.197";
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
  {{0x04, 0x65, 0x39, 0x22, 0x4B, 0x67, 0x80}, 7, "Tilak_Shetty",  "j8VCuLXccvNlpH8VCBWsfM2f0C02", false},
  {{0xFC, 0x01, 0x96, 0x81},                   4, "Hithesh_Jain",  "1OqBxj9j9BRdcAkamN3F1xT9d5k1", false},
  {{0x92, 0xE6, 0x40, 0x1D},                   4, "Karthik_Shenoy",     "Ru916gAxqxT5xLwE9qe9pXFtANb2", false}
};
const int userCount = sizeof(users) / sizeof(users[0]);

// === Musical Notes ===
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988

// === Setup Wi-Fi ===
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

// === MQTT Reconnect ===
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

// === UID Match ===
bool matchUID(byte* uid, byte size, const byte* ref, byte refSize) {
  if (size != refSize) return false;
  for (byte i = 0; i < size; i++) {
    if (uid[i] != ref[i]) return false;
  }
  return true;
}

// === Nokia Tune (Tilak) ===
void playNokiaTune(bool isEntry) {
  if (isEntry) {
    // Classic Nokia tune for entry
    int melody[] = {NOTE_E5, NOTE_D5, NOTE_FS4, NOTE_GS4, 
                   NOTE_CS5, NOTE_B4, NOTE_D4, NOTE_E4, 
                   NOTE_B4, NOTE_A4, NOTE_CS4, NOTE_E4,
                   NOTE_A4};
                   
    int noteDurations[] = {8, 8, 4, 4, 8, 8, 4, 4, 8, 8, 4, 4, 2};
    
    for (int i = 0; i < 13; i++) {
      int duration = 1000 / noteDurations[i];
      tone(BUZZER_PIN, melody[i], duration);
      delay(duration * 1.3);
      noTone(BUZZER_PIN);
    }
  } else {
    // Reversed Nokia tune for exit
    int melody[] = {NOTE_A4, NOTE_E4, NOTE_CS4, NOTE_A4, 
                   NOTE_E4, NOTE_D4, NOTE_B4, NOTE_A4, 
                   NOTE_GS4, NOTE_FS4, NOTE_D5, NOTE_E5};
                   
    int noteDurations[] = {2, 4, 4, 8, 8, 4, 4, 8, 8, 4, 8, 8};
    
    for (int i = 0; i < 12; i++) {
      int duration = 1000 / noteDurations[i];
      tone(BUZZER_PIN, melody[i], duration);
      delay(duration * 1.3);
      noTone(BUZZER_PIN);
    }
  }
}

// === Sa Re Ga Ma (Hithesh) ===
void playSaReGaMa(bool isEntry) {
  if (isEntry) {
    // Sa Re Ga Ma Pa Dha Ni Sa (Ascending)
    int melody[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, 
                    NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
                    
    int noteDurations[] = {4, 4, 4, 4, 4, 4, 4, 2};
    
    for (int i = 0; i < 8; i++) {
      int duration = 1000 / noteDurations[i];
      tone(BUZZER_PIN, melody[i], duration);
      delay(duration * 1.3);
      noTone(BUZZER_PIN);
    }
  } else {
    // Sa Ni Dha Pa Ma Ga Re Sa (Descending)
    int melody[] = {NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, 
                    NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4};
                    
    int noteDurations[] = {4, 4, 4, 4, 4, 4, 4, 2};
    
    for (int i = 0; i < 8; i++) {
      int duration = 1000 / noteDurations[i];
      tone(BUZZER_PIN, melody[i], duration);
      delay(duration * 1.3);
      noTone(BUZZER_PIN);
    }
  }
}

// === Game of Thrones Theme (Gaurav) ===
void playGameOfThrones(bool isEntry) {
  if (isEntry) {
    // Simplified Game of Thrones theme for entry
    int melody[] = {NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, 
                    NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4,
                    NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4,
                    NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4,
                    NOTE_G4};
                    
    int noteDurations[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 2};
    
    for (int i = 0; i < 17; i++) {
      int duration = 1000 / noteDurations[i];
      tone(BUZZER_PIN, melody[i], duration);
      delay(duration * 1.3);
      noTone(BUZZER_PIN);
    }
  } else {
    // Modified Game of Thrones theme for exit
    int melody[] = {NOTE_G4, NOTE_F4, NOTE_E4, NOTE_C4, 
                    NOTE_G4, NOTE_F4, NOTE_DS4, NOTE_C4,
                    NOTE_G4, NOTE_F4, NOTE_DS4, NOTE_C4,
                    NOTE_G4};
                    
    int noteDurations[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 2};
    
    for (int i = 0; i < 13; i++) {
      int duration = 1000 / noteDurations[i];
      tone(BUZZER_PIN, melody[i], duration);
      delay(duration * 1.3);
      noTone(BUZZER_PIN);
    }
  }
}

// === Error Tone ===
void playErrorTone() {
  tone(BUZZER_PIN, NOTE_C3, 200);
  delay(250);
  tone(BUZZER_PIN, NOTE_B2, 200);
  delay(250); 
  tone(BUZZER_PIN, NOTE_AS2, 400);
  delay(450);
  noTone(BUZZER_PIN);
}

// === Buzzer Melody per User ===
void playUserTone(String name, bool isEntry) {
  noTone(BUZZER_PIN);  // Reset

  if (name == "Hithesh_Jain") {
    playSaReGaMa(isEntry);
  }
  else if (name == "Tilak_Shetty") {
    playNokiaTune(isEntry);
  }
  else if (name == "Gaurav_BS") {
    playGameOfThrones(isEntry);
  }
  else {
    // Unknown card – error tone
    playErrorTone();
  }
}


// === Setup ===
// ✅ [NO CHANGES HERE] — All your includes, defines, note frequencies, and custom functions are perfect as-is

// === Setup ===
void setup() {
  Serial.begin(115200);
  SPI.begin(5, 19, 23, 18); // SCK, MISO, MOSI, SS
  rfid.PCD_Init();
  pinMode(BUZZER_PIN, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  Serial.println("Tap your card to toggle entry/exit...");
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

// === Main Loop ===
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

      // Play user-specific buzzer tone
      playUserTone(users[i].name, users[i].isInside);

      // Send MQTT messages
      String greeting = users[i].isInside ? "Welcome " + users[i].name : "Bye " + users[i].name;
      client.publish("rfid/greeting", greeting.c_str());

      String message = "{ \"uid\": \"" + users[i].idString + "\", \"action\": \"" + action + "\", \"name\": \"" + users[i].name + "\" }";
      client.publish("rfid/status", message.c_str());

      Serial.println(greeting);
      Serial.println("Publishing to MQTT:");
      Serial.println("  Topic: rfid/status");
      Serial.println("  Message: " + message);
      break;
    }
  }

  if (!userFound) {
    Serial.println("Unknown card scanned.");
    playErrorTone();
  }

  while (rfid.PICC_IsNewCardPresent() || rfid.PICC_ReadCardSerial()) {
    delay(100);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(5000);  // Prevent double scan
}
