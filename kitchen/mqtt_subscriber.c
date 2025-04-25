#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <unistd.h>   // for sleep()
#include <fcntl.h>    // for open()

#define PIPE_PATH "/tmp/mqtt_kitchen"

char last_gas[32] = "";
char last_flame[32] = "";

// Callback when connected to MQTT broker
void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc == 0) {
        printf("? Connected to MQTT broker.\n");
        mosquitto_subscribe(mosq, NULL, "home/gas", 0);
        mosquitto_subscribe(mosq, NULL, "home/flame", 0);
        printf("? Subscribed to: home/gas, home/flame\n");
    } else {
        printf("? Failed to connect, code: %d\n", rc);
    }
}

// Callback when message received
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    const char *topic = msg->topic;
    const char *payload = (char *)msg->payload;

    printf("? Received: Topic: %s | Message: %s\n", topic, payload);

    if (strcmp(topic, "home/gas") == 0) {
        strncpy(last_gas, payload, sizeof(last_gas));
    } else if (strcmp(topic, "home/flame") == 0) {
        strncpy(last_flame, payload, sizeof(last_flame));
    }

    // Only send when both values are available
    if (strlen(last_gas) > 0 && strlen(last_flame) > 0) {
        char pipe_data[256];
        snprintf(pipe_data, sizeof(pipe_data), "gas:%s fire:%s", last_gas, last_flame);

        int pipe_fd = open(PIPE_PATH, O_WRONLY | O_NONBLOCK);
        if (pipe_fd == -1) {
            fprintf(stderr, "??  Could not open pipe. Is the Python script running?\n");
        } else {
            write(pipe_fd, pipe_data, strlen(pipe_data));
            close(pipe_fd);
            printf("? Sent to Python via pipe: %s\n", pipe_data);
        }
    }
}

int main() {
    mosquitto_lib_init();
    struct mosquitto *mosq = mosquitto_new("pi-subscriber", true, NULL);

    if (!mosq) {
        fprintf(stderr, "? Failed to create Mosquitto client.\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    const char* broker = "172.16.10.197";
    int rc;

    while ((rc = mosquitto_connect(mosq, broker, 1883, 60)) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "? MQTT connection failed: %s. Retrying...\n", mosquitto_strerror(rc));
        sleep(5);
    }

    printf("? Monitoring MQTT topics...\n");

    if (access(PIPE_PATH, F_OK) == -1) {
        printf("??  Pipe %s not found. Please start the Python reader first.\n", PIPE_PATH);
    }

    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
