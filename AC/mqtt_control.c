#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>

#define BROKER_IP "192.168.91.243"  // Replace with the Raspberry Pi IP
#define BROKER_PORT 1883
#define CLIENT_ID "RaspberryPiClient"

// Function to handle incoming messages
void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    if (strcmp(message->topic, "home/room1/temperature") == 0) {
        printf("Received temperature: %s\n", (char *)message->payload);
    } 
    else if (strcmp(message->topic, "home/room1/warning") == 0) {
        printf("Warning: %s\n", (char *)message->payload);
        char response[4];
        printf("Do you want to turn ON the AC? (yes/no): ");
        scanf("%s", response);

        // Publish response to control AC (LED)
        if (strcmp(response, "yes") == 0) {
            mosquitto_publish(mosq, NULL, "home/room1/ac_control", strlen("yes"), "yes", 0, false);
            printf("AC turned ON\n");
        } else {
            mosquitto_publish(mosq, NULL, "home/room1/ac_control", strlen("no"), "no", 0, false);
            printf("AC turned OFF\n");
        }
    }
}

int main() {
    struct mosquitto *mosq;
    int ret;

    // Initialize the MQTT client
    mosquitto_lib_init();

    mosq = mosquitto_new(CLIENT_ID, true, NULL);
    if (!mosq) {
        fprintf(stderr, "Error: Unable to create Mosquitto client.\n");
        return 1;
    }

    // Set the message callback
    mosquitto_message_callback_set(mosq, message_callback);

    // Connect to the MQTT broker
    ret = mosquitto_connect(mosq, BROKER_IP, BROKER_PORT, 60);
    if (ret) {
        fprintf(stderr, "Unable to connect to MQTT broker.\n");
        return 1;
    }

    // Subscribe to the topics
    mosquitto_subscribe(mosq, NULL, "home/room1/temperature", 0);
    mosquitto_subscribe(mosq, NULL, "home/room1/warning", 0);

    // Start the loop to listen for messages
    while (1) {
        ret = mosquitto_loop(mosq, -1, 1);
        if (ret) {
            fprintf(stderr, "Error in MQTT loop: %d\n", ret);
            break;
        }
    }

    // Clean up
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
