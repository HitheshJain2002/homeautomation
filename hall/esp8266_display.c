#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// TFT pins
#define TFT_CS   4
#define TFT_DC   5
#define TFT_RST  2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Wi-Fi & MQTT
const char* ssid = "Qwerty";
const char* password = "qwerty@123";
const char* mqtt_server = "172.16.10.197";
const char* greeting_topic = "rfid/greeting";
const char* status_topic = "rfid/status";

// OpenWeatherMap
const char* weather_api_key = "fe6d456f177e6fb56b7731357c1debfd";
const char* city = "Nitte";
const char* country_code = "IN";

// NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiClient weatherClient;

// Time
char dateString[20];
char timeString[10];
unsigned long lastTimeUpdate = 0;
const unsigned long timeUpdateInterval = 60000; // update once per minute

// Display state
enum DisplayState { GREETING, USER_TABLE, WEATHER };
DisplayState currentState = WEATHER;
unsigned long stateChangeTime = 0;
const unsigned long greetingDuration = 3000;
const unsigned long userTableDuration = 15000;
const unsigned long weatherUpdateInterval = 300000;

// Users
#define MAX_USERS 5
struct User {
  String name, status;
  bool isActive;
} users[MAX_USERS] = {
  {"Tilak Shetty", "Unknown", false},
  {"Hithesh Jain", "Unknown", false},
  {"Karthik Shenoy", "Unknown", false},
  {"Gaurav BS", "Unknown", false},
  {"Guest", "Unknown", false}
};

// Weather
struct WeatherData {
  String description;
  float temperature, humidity, windSpeed;
  String cityName;
  unsigned long lastUpdate;
} weatherData;

char greetingMessage[100] = "Waiting for activity...";
bool newGreeting = false, newUserStatus = false, shouldUpdateWeather = true;

void setupWiFi() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); // Newline after dots
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());  // This prints only once
}


void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while (time(nullptr) < 8 * 3600 * 2) delay(500);
  updateTimeStrings();
}

void updateTimeStrings() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  sprintf(dateString, "%02d-%02d-%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
  sprintf(timeString, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min); // No seconds
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  if (strcmp(topic, greeting_topic) == 0) {
    strncpy(greetingMessage, (char*)payload, sizeof(greetingMessage) - 1);
    newGreeting = true;
    currentState = GREETING;
    stateChangeTime = millis();
  } else if (strcmp(topic, status_topic) == 0) {
    DynamicJsonDocument doc(1024);
    if (!deserializeJson(doc, payload)) {
      String name = doc["name"], action = doc["action"];
      for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].name == name) {
          users[i].status = action;
          users[i].isActive = true;
          newUserStatus = true;
          break;
        }
      }
    }
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP8266Display")) {
      mqttClient.subscribe(greeting_topic);
      mqttClient.subscribe(status_topic);
    } else {
      delay(5000);
    }
  }
}

void updateWeatherData() {
  HTTPClient http;
  String url = String("http://api.openweathermap.org/data/2.5/weather?q=") + city + "," + country_code + "&units=metric&APPID=" + weather_api_key;
  http.begin(weatherClient, url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument doc(1024);
    if (!deserializeJson(doc, http.getString())) {
      weatherData.description = doc["weather"][0]["description"].as<String>();
      weatherData.temperature = doc["main"]["temp"];
      weatherData.humidity = doc["main"]["humidity"];
      weatherData.windSpeed = doc["wind"]["speed"];
      weatherData.cityName = doc["name"].as<String>();
      weatherData.lastUpdate = millis();
      weatherData.description.toLowerCase();
      weatherData.description[0] = toupper(weatherData.description[0]);
    }
  }
  http.end();
  shouldUpdateWeather = false;
}

void displayGreeting() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  String msg(greetingMessage);
  tft.setCursor(40, 90);
  tft.println(msg);
}

void displayUserTable() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("User Status");
  tft.drawLine(0, 35, 320, 35, ILI9341_WHITE);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(10, 45); tft.print("NAME");
  tft.setCursor(180, 45); tft.print("STATUS");
  tft.drawLine(0, 60, 320, 60, ILI9341_WHITE);

  int y = 70;
  tft.setTextSize(2);
  for (int i = 0; i < MAX_USERS; i++) {
    if (users[i].isActive) {
      tft.setTextColor(users[i].status == "Home" ? ILI9341_GREEN : ILI9341_RED);
      tft.setCursor(10, y); tft.print(users[i].name);
      tft.setCursor(180, y); tft.print(users[i].status);
      y += 30;
    }
  }
  if (y == 70) {
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 100);
    tft.println("No active users");
  }
}


void displayWeather() {
  tft.fillScreen(ILI9341_BLACK);




  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 10); tft.print(dateString);
  tft.setCursor(240, 10); tft.print(timeString);

  tft.setCursor(110, 40);
  tft.setTextSize(2); tft.println(city);

  tft.setCursor(90, 80);
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_YELLOW);
  tft.print(String(weatherData.temperature, 1)); tft.print(" C");

  tft.setCursor(10, 140);
  tft.setTextSize(2); tft.setTextColor(ILI9341_GREEN);
  tft.println(weatherData.description);

  // ✅ Humidity and Wind on the same line
  tft.setTextColor(ILI9341_PINK);
  tft.setTextSize(2);
  tft.setCursor(10, 180);
  tft.print("Humidity: "); tft.print(weatherData.humidity); tft.print("%");
  tft.setCursor(10, 200); // shifted to the right for wind
  tft.print("Wind: "); tft.print(weatherData.windSpeed); tft.print(" m/s");

  // ✅ Updated line smaller and lower

}

void setup() {
  tft.begin(); tft.setRotation(1); tft.fillScreen(ILI9341_BLACK);
  setupWiFi(); setupTime();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(2);
  tft.setCursor(40, 120); tft.println("Initializing...");
}

void loop() {
  unsigned long currentTime = millis();

  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();

  if (currentTime - lastTimeUpdate >= timeUpdateInterval) {
    updateTimeStrings();
    lastTimeUpdate += timeUpdateInterval;
    if (currentState == WEATHER) displayWeather();
  }

  if (currentTime - weatherData.lastUpdate > weatherUpdateInterval || shouldUpdateWeather)
    updateWeatherData();

  switch (currentState) {
    case GREETING:
      if (newGreeting) {
        displayGreeting();
        newGreeting = false;
      }
      if (currentTime - stateChangeTime > greetingDuration) {
        currentState = USER_TABLE;
        stateChangeTime = currentTime;
        displayUserTable();
      }
      break;

    case USER_TABLE:
      if (newUserStatus) {
        displayUserTable();
        newUserStatus = false;
      }
      if (currentTime - stateChangeTime > userTableDuration) {
        currentState = WEATHER;
        displayWeather();
      }
      break;

    case WEATHER:
      // nothing to do, update handled by interval
      break;
  }

  delay(1000);
}
