#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
 
#define BROKER "172.16.10.197"   // MQTT broker IP address
#define PORT 1883               // MQTT port
#define TOPIC "rfid/status"     // MQTT topic to subscribe to
#define PIPE_PATH "/tmp/uid_pipe"  // Path to the named pipe
 
// Callback when connected to the MQTT broker
void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc == 0) {
        printf("Connected to MQTT broker.\n");
        mosquitto_subscribe(mosq, NULL, TOPIC, 0);  // Subscribe to the topic
    } else {
        printf("Failed to connect, return code %d\n", rc);
    }
}
 
// Callback when a message is received
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    printf("Received message on topic %s: %s\n", msg->topic, (char *)msg->payload);
 
    // Check if the pipe exists, otherwise attempt to create it
    if (access(PIPE_PATH, F_OK) == -1) {
        if (mkfifo(PIPE_PATH, 0666) == -1) {
            perror("Failed to create pipe");
            return;
        }
    }
 
    // Open the named pipe for writing
    int pipe_fd = open(PIPE_PATH, O_WRONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        return;
    }
 
    // Write the JSON payload to the pipe
    ssize_t bytes_written = write(pipe_fd, msg->payload, strlen(msg->payload));
    if (bytes_written == -1) {
        perror("Failed to write to pipe");
    } else {
        printf("Sent message to pipe: %s\n", (char *)msg->payload);
    }
 
    // Close the pipe
    close(pipe_fd);
}
 
int main() {
    mosquitto_lib_init();  // Initialize the Mosquitto library
 
    // Create a new Mosquitto client
    struct mosquitto *mosq = mosquitto_new("mqtt_subscriber", true, NULL);
    if (!mosq) {
        fprintf(stderr, "Failed to create Mosquitto client.\n");
        mosquitto_lib_cleanup();
        return 1;
    }
 
    // Set the callback functions for connection and message
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
 
    // Connect to the MQTT broker
    if (mosquitto_connect(mosq, BROKER, PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Unable to connect to broker.\n");
        return 1;
    }
 
    // Start the Mosquitto network loop in a separate thread
    mosquitto_loop_start(mosq);
 
    printf("Listening for messages on topic '%s'...\n", TOPIC);
 
    // Run the subscriber loop
    while (1) {
        // Infinite loop to keep the subscriber active
        sleep(1);  // Sleep for 1 second
    }
 
    // Cleanup and close
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
