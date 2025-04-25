#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mosquitto.h>

#define MQTT_HOST "52b1c31363bc477e8460c34c2532a968.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "hithesh"
#define MQTT_PASS "Qwerty@123"
#define MQTT_TOPIC "pi/led"

#define PIPE_PATH "/tmp/mqtt_pipe"

void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    if (rc == 0) {
        printf("? MQTT connected successfully!\n");
    } else {
        printf("? MQTT connection failed with code %d\n", rc);
    }
}

int main() {
    printf("? Starting MQTT Pipe Handler...\n");

    // ? Initialize Mosquitto
    mosquitto_lib_init();
    struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);

    if (!mosq) {
        fprintf(stderr, "? Failed to create mosquitto instance\n");
        return 1;
    }

    // ? Set MQTT credentials
    mosquitto_username_pw_set(mosq, MQTT_USER, MQTT_PASS);

    // ? Set TLS certificate (system default CA certs)
    mosquitto_tls_set(mosq, "/etc/ssl/certs/ca-certificates.crt", NULL, NULL, NULL, NULL);

    // ? Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);

    // ? Connect to MQTT broker
    int rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "? MQTT connect error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    // ? Start loop
    mosquitto_loop_start(mosq);

    // ? Open named pipe for reading
    int fd;
    char buffer[10];

    printf("? Waiting for messages on pipe: %s\n", PIPE_PATH);

    while (1) {
        fd = open(PIPE_PATH, O_RDONLY);
        if (fd == -1) {
            perror("? Failed to open pipe");
            sleep(1);
            continue;
        }

        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // Null-terminate
            printf("? Received from pipe: %s\n", buffer);

            mosquitto_publish(mosq, NULL, MQTT_TOPIC, strlen(buffer), buffer, 0, false);
            printf("? Sent to MQTT topic [%s]: %s\n", MQTT_TOPIC, buffer);
        }

        close(fd);
        sleep(1);
    }

    // ? Cleanup
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
