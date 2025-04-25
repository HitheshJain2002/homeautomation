#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <unistd.h>   // for sleep()

// Callback when connected to MQTT broker
void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc == 0) {
        printf("? Connected to MQTT broker successfully.\n");

        // Subscribe to topics
        mosquitto_subscribe(mosq, NULL, "home/gas", 0);
        mosquitto_subscribe(mosq, NULL, "home/flame", 0);
        printf("? Subscribed to topics: home/gas, home/flame\n");
    } else {
        printf("? Failed to connect, return code %d\n", rc);
    }
}

// Callback when a message is received
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    printf("? Topic: %s | Message: %s\n", msg->topic, (char *)msg->payload);
}

int main() {
    // Initialize the Mosquitto library
    mosquitto_lib_init();

    // Create a new Mosquitto client
    struct mosquitto *mosq = mosquitto_new("pi-subscriber", true, NULL);
    if (!mosq) {
        fprintf(stderr, "? Failed to create Mosquitto instance.\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    // Set callback functions
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    // Print connection attempt
    printf("? Connecting to MQTT broker...\n");

    // Connect to broker
    int rc;
    while ((rc = mosquitto_connect(mosq, "broker.hivemq.com", 1883, 60)) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "? Connection failed: %s. Retrying in 2s...\n", mosquitto_strerror(rc));
        sleep(2);  // Wait before retrying
    }

    // Blocking loop to process network traffic and callbacks
    mosquitto_loop_forever(mosq, -1, 1);

    // Cleanup
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
