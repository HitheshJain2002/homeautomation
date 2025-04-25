#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define MQTT_HOST "localhost"
#define MQTT_PORT 1883
#define MQTT_TOPIC "home/temp"
#define PIPE_PATH "/tmp/mqtt_pipe"


void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    if (message->payloadlen) {
        printf("Received: %s\n", (char *)message->payload);

        // Send to named pipe
        int fd = open(PIPE_PATH, O_WRONLY | O_NONBLOCK);
        if (fd != -1) {
            write(fd, message->payload, message->payloadlen);
            write(fd, "\n", 1);
            close(fd);
        } else {
            perror("Pipe open failed");
        }
    }
}

int main() {
    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new("raspi_sub", true, NULL);
    if (!mosq) {
        fprintf(stderr, "Failed to create mosquitto instance\n");
        return 1;
    }

    mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
    mosquitto_subscribe(mosq, NULL, MQTT_TOPIC, 0);
    mosquitto_message_callback_set(mosq, on_message);

    // Create the pipe if not exists
    mkfifo(PIPE_PATH, 0666);

    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
