#include <Servo.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
 
#define GAS_SENSOR_PIN A0      // MQ-135 analog pin
#define FLAME_SENSOR_PIN 5     // D1 = GPIO5
#define SERVO_PIN 4            // D2 = GPIO4
#define BUZZER_PIN 12          // D6 = GPIO12
 
// WiFi & MQTT Config
const char* ssid = "Qwerty";
const char* password = "qwerty@123";
const char* mqtt_server = "172.16.10.197"; // MQTT broker IP
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP8266_Kitchen";  // Unique name
 
// Timing constants
const unsigned long NORMAL_PUBLISH_INTERVAL = 120000;  // 2 mins = 120,000 ms
 
// Gas sensor mapping values
const int GAS_MIN_VALUE = 25;  // 0% on percentage scale
const int GAS_MAX_VALUE = 80;  // 100% on percentage scale
const int GAS_DANGER_THRESHOLD = 60;  // Gas percentage threshold for danger
 
// Tracking variables
unsigned long lastPublishTime = 0;
boolean dangerDetectedSinceLastPublish = false;
unsigned long lastReconnectAttempt = 0;
const long reconnectInterval = 5000;  // Try to reconnect every 5 seconds
int lastPublishedGasPercentage = 0;   // Track last published gas percentage
int lastPublishedFlameValue = 0;      // Track last published flame value
 
WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo;
 
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
 
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected");
    Serial.print("→ IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi connection FAILED");
  }
}
 
boolean reconnect() {
  Serial.print("Attempting MQTT connection... ");
  if (client.connect(mqtt_client_id, NULL, NULL, "home/status", 1, true, "offline")) {
    Serial.println("connected");
    client.publish("home/status", "online", true);
    return true;
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" - will retry");
    return false;
  }
}

// Convert gas sensor value to percentage (0-100%)
int gasToPercentage(int gasValue) {
  // Constrain raw value to our defined range
  int constrainedValue = constrain(gasValue, GAS_MIN_VALUE, GAS_MAX_VALUE);
  
  // Map the constrained value to percentage (0-100)
  int percentage = map(constrainedValue, GAS_MIN_VALUE, GAS_MAX_VALUE, 0, 100);
  
  return percentage;
}
 
void publishSensorValues(int gasPercentage, int flameValue, boolean forceSend) {
  unsigned long currentTime = millis();
  
  // Calculate percentage difference
  int percentageDifference = abs(gasPercentage - lastPublishedGasPercentage);
  
  // Check if flame state has changed
  boolean flameChanged = (flameValue != lastPublishedFlameValue);
  
  // Determine if we should publish based on conditions
  boolean shouldPublish = 
      forceSend || 
      (currentTime - lastPublishTime >= NORMAL_PUBLISH_INTERVAL) || 
      (percentageDifference > 5) ||  // Publish if percentage changes by more than 5%
      flameChanged;
 
  if (shouldPublish && client.connected()) {
    char gasStr[10], flameStr[10];
    
    // Format gas value as percentage
    sprintf(gasStr, "%d", gasPercentage);
    sprintf(flameStr, "%d", flameValue);
 
    client.publish("home/gas", gasStr);
    client.publish("home/flame", flameStr);
 
    Serial.println("✓ Published to MQTT:");
    Serial.print("→ Gas: ");
    Serial.print(gasStr);
    Serial.print(" | Flame: ");
    Serial.println(flameStr);
 
    // Update tracking variables
    lastPublishTime = currentTime;
    lastPublishedGasPercentage = gasPercentage;
    lastPublishedFlameValue = flameValue;
    dangerDetectedSinceLastPublish = false;
  }
}

// Function to activate servo alarm pattern
void activateServoAlarm() {
  // Servo alert movement
  myServo.write(180);
  delay(500);
  myServo.write(0);
  delay(500);
  myServo.write(180);
  delay(500);
  myServo.write(0);
}
 
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- Kitchen Safety System Starting ---");
 
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
 
  myServo.attach(SERVO_PIN);
  myServo.write(0);  // Default position
 
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  reconnect();
}
 
void loop() {
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }
 
  int rawGasValue = analogRead(GAS_SENSOR_PIN);
  int gasPercentage = gasToPercentage(rawGasValue);
  int flameValue = digitalRead(FLAME_SENSOR_PIN);
  boolean flameDetected = (flameValue == HIGH); // Flame detected if HIGH
 
  Serial.print("Gas: ");
  Serial.print(rawGasValue);
  Serial.print(" (");
  Serial.print(gasPercentage);
  Serial.print("%) | Flame: ");
  Serial.println(flameDetected ? "DETECTED!" : "None");
 
  // Check for high gas levels
  boolean highGas = (gasPercentage > GAS_DANGER_THRESHOLD);
  
  // Consider danger when either gas is high OR flame is detected
  boolean dangerDetected = highGas || flameDetected;
  boolean forceSend = dangerDetected && !dangerDetectedSinceLastPublish;
 
  if (forceSend) {
    dangerDetectedSinceLastPublish = true;
  }

  publishSensorValues(gasPercentage, flameDetected ? 1 : 0, forceSend);
  
  // Handle high gas level alert
  if (highGas) {
    Serial.println("!!! ALERT: High gas levels detected!");
    digitalWrite(BUZZER_PIN, HIGH);
    activateServoAlarm();
  }
  
  // Handle flame detection alert
  if (flameDetected) {
    Serial.println("!!! ALERT: Flame detected!");
    digitalWrite(BUZZER_PIN, HIGH);
    activateServoAlarm();
  }
  
  // If no danger, turn off alerts
  if (!dangerDetected) {
    digitalWrite(BUZZER_PIN, LOW);
    myServo.write(0);
  }
 
  delay(3000); // Read every 3 seconds
}