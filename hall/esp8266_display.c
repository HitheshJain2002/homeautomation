#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Define pins
#define TFT_CS   4  // Chip select
#define TFT_DC   5  // Data/command
#define TFT_RST  2  // Reset

// Create display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Wi-Fi and MQTT settings
const char* ssid = "Qwerty";
const char* password = "qwerty@123";
const char* mqtt_server = "172.16.10.197";
const int mqtt_port = 1883;
const char* mqtt_topic = "rfid/greeting";

WiFiClient espClient;
PubSubClient client(espClient);

// Buffer to store received greeting
char greeting[100] = "Waiting for greeting...";
bool updateDisplay = true;

// Connect to Wi-Fi
void setupWiFi() {
  Serial.begin(115200);
  Serial.println("Connecting to WiFi...");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  // Clear the greeting buffer
  memset(greeting, 0, sizeof(greeting));
  
  // Copy the payload to the greeting buffer
  if (length < sizeof(greeting)) {
    memcpy(greeting, payload, length);
  } else {
    memcpy(greeting, payload, sizeof(greeting) - 1);
  }
  
  Serial.println(greeting);
  updateDisplay = true;
}

// Reconnect to MQTT broker if connection is lost
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Initialize display
  tft.begin();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(ILI9341_BLACK);
  
  // Initialize serial, Wi-Fi and MQTT
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Show initial message
  updateGreetingDisplay();
}

void updateGreetingDisplay() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(20, 100);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  
  // If greeting is too long, split it into multiple lines
  int len = strlen(greeting);
  if (len > 20) {
    char line1[30], line2[30];
    strncpy(line1, greeting, 20);
    line1[20] = '\0';
    strncpy(line2, greeting + 20, 29);
    line2[29] = '\0';
    
    tft.println(line1);
    tft.setCursor(20, 130);
    tft.println(line2);
  } else {
    tft.println(greeting);
  }
}

void loop() {
  // Maintain MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Update display if new greeting received
  if (updateDisplay) {
    updateGreetingDisplay();
    updateDisplay = false;
  }
}