#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <curl/curl.h>
#include <unistd.h>

#define LED_PIN 0  // WiringPi pin 0 = GPIO17
#define FIREBASE_URL "https://habiteck-45fcb-default-rtdb.firebaseio.com/Users/pb6YvUNL4HfSBOa3vAb0r9vYOjb2/rooms/Child%20Bedroom/devices/-ONYXmqFSf0ZtlzVIj94/on.json"

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    strcat((char *)userdata, (char *)ptr);
    return size * nmemb;
}

int fetch_led_state() {
    CURL *curl = curl_easy_init();
    char response[100] = {0};

    if (!curl) return -1;

    curl_easy_setopt(curl, CURLOPT_URL, FIREBASE_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return -1;

    return (strstr(response, "true") || strstr(response, "1")) ? 1 : 0;
}

int main() {
    wiringPiSetup();
    pinMode(LED_PIN, OUTPUT);

    printf("Monitoring LED state from Firebase...\n");

    int last_state = -1;

    while (1) {
        int state = fetch_led_state();
        if (state != -1 && state != last_state) {
            digitalWrite(LED_PIN, state);
            printf("LED %s\n", state ? "ON" : "OFF");
            last_state = state;
        }
        sleep(1);
    }

    return 0;
}
