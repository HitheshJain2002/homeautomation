#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mosquitto.h>

void callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    printf("Received from ESP32: %s\n", (char *)msg->payload);
    
    // Send response back to ESP32
    mosquitto_publish(mosq, NULL, "raspberrypi/test", strlen("Hello ESP32!"), "Hello ESP32!", 0, false);
}

int main() {
    struct mosquitto *mosq;
    mosquitto_lib_init();
    mosq = mosquitto_new("raspberryPi", true, NULL);

    mosquitto_connect(mosq, "broker.hivemq.com", 1883, 60);
    mosquitto_subscribe(mosq, NULL, "esp32/test", 0);
    mosquitto_message_callback_set(mosq, callback);

    mosquitto_loop_forever(mosq, -1, 1);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
