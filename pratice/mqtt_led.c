	#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

// ? MQTT Broker Details
#define MQTT_HOST "52b1c31363bc477e8460c34c2532a968.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "hithesh"
#define MQTT_PASS "Qwerty@123"
#define MQTT_TOPIC "pi/led"

// ? Function to send MQTT message
void send_mqtt_message(const char *state) {
    struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);
    mosquitto_username_pw_set(mosq, MQTT_USER, MQTT_PASS);
    mosquitto_tls_set(mosq, "/etc/ssl/certs/ca-certificates.crt", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    
    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        printf("Failed to connect to MQTT\n");
        return;
    }

    mosquitto_publish(mosq, NULL, MQTT_TOPIC, strlen(state), state, 0, false);
    printf("Sent MQTT message: %s\n", state);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
}

// ? Function to fetch LED state from Firebase
char* fetch_firebase_led_state() {
    system("curl -s 'https://home-automation-66846-default-rtdb.asia-southeast1.firebasedatabase.app/led.json' > led_state.txt");
    
    FILE *file = fopen("led_state.txt", "r");
    if (!file) return NULL;

    static char state[4];
    fgets(state, sizeof(state), file);
    fclose(file);

    if (strstr(state, "ON")) return "ON";
    if (strstr(state, "OFF")) return "OFF";
    return NULL;
}

int main() {
    printf("Starting Raspberry Pi LED Controller...\n");

    char *last_state = "";

    while (1) {
        char *new_state = fetch_firebase_led_state();
        if (new_state && strcmp(new_state, last_state) != 0) {
            send_mqtt_message(new_state);
            last_state = new_state;
        }
        sleep(2);
    }

    return 0;
} 
