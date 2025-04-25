#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>  // For mkfifo()
#include <pthread.h>
#include <signal.h>
#include <ctype.h>

// === Named Pipes ===
#define PIPE_TEMP  "/tmp/temp_pipe"
#define PIPE_LED   "/tmp/led_pipe"
#define PIPE_SMOKE "/tmp/smoke_pipe"
#define PIPE_AC    "/tmp/ac_pipe"
#define PIPE_SERVO "/tmp/servo_pipe"
#define PIPE_GEYSER "/tmp/geyser_pipe"
#define PIPE_SOUND  "/tmp/sound_pipe"
#define PIPE_STATUS "/tmp/status_pipe"

// === MQTT Topics (aligned with ESP8266) ===
#define TOPIC_LED_CMD    "room/led"       // Commands to LED
#define TOPIC_LED_STATUS "room/led_status"      // Status from LED (ESP subscription topic)
#define TOPIC_TEMP       "room/temp"       // Temperature data
#define TOPIC_AC         "room/AC"        // AC control
#define TOPIC_SMOKE      "room/smoke"      // Smoke sensor data
#define TOPIC_SERVO      "servo/angle"     // Servo control
#define TOPIC_GEYSER     "room/geyser"     // Geyser control
#define TOPIC_SOUND      "room/sound"      // Sound sensor data
#define TOPIC_STATUS     "room/status"     // Device status

// === Global Variables ===
struct mosquitto *mosq = NULL;
pthread_t reader_thread;
volatile int running = 1;
const char *mqtt_host = "172.16.10.197";
int mqtt_port = 1883;
const char *client_id = "MQTT_Pipe_Bridge";
static int req_mid = 0;
static char last_servo_payload[32] = "";
static char last_ac_payload[32] = "";


// === Ensure the pipe exists or create it ===
void ensure_pipe_exists(const char *pipe_path) {
    if (access(pipe_path, F_OK) == -1) {
        if (mkfifo(pipe_path, 0666) != 0) {
            fprintf(stderr, "? Failed to create pipe %s: %s\n", pipe_path, strerror(errno));
        } else {
            printf("? Created missing pipe: %s\n", pipe_path);
        }
    }
}

// === Write message to pipe ===
void write_to_pipe(const char *pipe_path, const char *msg, const char *topic) {
    ensure_pipe_exists(pipe_path);  // Ensure pipe exists before writing
    int fd = open(pipe_path, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "? Could not open pipe [%s] for topic [%s]: %s\n", pipe_path, topic, strerror(errno));
        return;
    }
   
    // For boolean values, convert "true"/"false" to "1"/"0" for consistency
    if (strcmp(topic, TOPIC_LED_CMD) == 0 || strcmp(topic, TOPIC_AC) == 0 ||
        strcmp(topic, TOPIC_GEYSER) == 0) {
        // Handle boolean conversion
        if (strcasecmp(msg, "true") == 0 || strcmp(msg, "1") == 0 || strcasecmp(msg, "on") == 0 ||
            strcasecmp(msg, "led_on") == 0) {
            dprintf(fd, "1\n");
            printf("? Written to [%s] | Topic: %s | Msg: 1 (Boolean TRUE)\n", pipe_path, topic);
        } else {
            dprintf(fd, "0\n");
            printf("? Written to [%s] | Topic: %s | Msg: 0 (Boolean FALSE)\n", pipe_path, topic);
        }
    } else {
        // For non-boolean values
        dprintf(fd, "%s\n", msg);
        printf("? Written to [%s] | Topic: %s | Msg: %s\n", pipe_path, topic, msg);
    }
   
    close(fd);
}
bool interpret_led_state(const char* input) {
    // Convert input to lowercase for case-insensitive comparison
    char lower_input[16];
    size_t i;
    for (i = 0; i < sizeof(lower_input) - 1 && input[i]; i++) {
        lower_input[i] = tolower((unsigned char)input[i]);
    }
    lower_input[i] = '\0';

    // Check for true equivalents
    if (strcmp(lower_input, "1") == 0 || strcmp(lower_input, "true") == 0 || strcmp(lower_input, "on") == 0) {
        return true;
    }

    // Check for false equivalents
    if (strcmp(lower_input, "0") == 0 || strcmp(lower_input, "false") == 0 || strcmp(lower_input, "off") == 0) {
        return false;
    }

    // Default behavior: treat unrecognized input as false
    return false;
}

// === Trim whitespace from a string ===
char* trim(char* str) {
    if (str == NULL) return NULL;
   
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
   
    if(*str == 0) return str;  // All spaces
   
    // Trim trailing space
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
   
    // Write new null terminator
    *(end+1) = 0;
   
    return str;
}

// === Parse JSON value (simple version) ===
char* extract_json_value(const char* json, const char* key) {
    static char value[256];
    char search_key[64];
    char* start;
    char* end;
   
    // Format search key with quotes and colon
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
   
    // Find key in JSON
    start = strstr(json, search_key);
    if (!start) return NULL;
   
    // Skip past key and whitespace
    start += strlen(search_key);
    while (*start && isspace(*start)) start++;
   
    // Check if value is quoted string
    if (*start == '"') {
        start++; // Skip opening quote
        end = strchr(start, '"');
        if (!end) return NULL;
       
        strncpy(value, start, end - start);
        value[end - start] = '\0';
    } else {
        // Value is number, boolean, or null
        end = start;
        while (*end && *end != ',' && *end != '}') end++;
       
        strncpy(value, start, end - start);
        value[end - start] = '\0';
        trim(value);
    }
   
    return value;
}

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    if (msg == NULL || msg->payload == NULL || msg->mid == req_mid) return;
    
        // Ensure payload is null-terminated
    char buffer[msg->payloadlen + 1];
    memcpy(buffer, msg->payload, msg->payloadlen);
    buffer[msg->payloadlen] = '\0';
   
    printf("? [Topic: %s] Payload: %s\n", msg->topic, buffer);
    
        // Topic-to-pipe routing
    if (strcmp(msg->topic, TOPIC_TEMP) == 0) {
        write_to_pipe(PIPE_TEMP, buffer, msg->topic);
       
        // Check for temperature alerts in JSON
        char* temp_value = extract_json_value(buffer, "temperature");
        if (temp_value) {
            float temperature = atof(temp_value);
            printf("?? Temperature: %.1fÂ°C\n", temperature);
        }
    }

    else if (strcmp(msg->topic, TOPIC_SMOKE) == 0) {
        // Extract smoke value from JSON
        char* smoke_value = extract_json_value(buffer, "smoke");
        if (smoke_value) {
            write_to_pipe(PIPE_SMOKE, smoke_value, msg->topic);
           
            if (atoi(smoke_value) != 0) {
                printf("? Smoke Detected!\n");
            }
        } else {
            write_to_pipe(PIPE_SMOKE, buffer, msg->topic);
        }
    }
    
}
// === Read from pipes and publish to MQTT ===
void *pipe_reader_thread(void *arg) {
    struct mosquitto *mosq = (struct mosquitto *)arg;
   
    // Setup for non-blocking reads
    fd_set readfds;
    int maxfd = 0;
    int fd_led = -1, fd_ac = -1, fd_temp = -1, fd_smoke = -1;
    int fd_servo = -1, fd_geyser = -1, fd_sound = -1, fd_status = -1;
    char buffer[1024];
   
    // Ensure all pipes exist
    ensure_pipe_exists(PIPE_LED);
    ensure_pipe_exists(PIPE_AC);
    ensure_pipe_exists(PIPE_TEMP);
    ensure_pipe_exists(PIPE_SMOKE);
    ensure_pipe_exists(PIPE_SERVO);
    ensure_pipe_exists(PIPE_GEYSER);
    ensure_pipe_exists(PIPE_SOUND);
    ensure_pipe_exists(PIPE_STATUS);
   
    // Open all pipes for reading
    fd_led = open(PIPE_LED, O_RDONLY | O_NONBLOCK);
    fd_ac = open(PIPE_AC, O_RDONLY | O_NONBLOCK);
    fd_temp = open(PIPE_TEMP, O_RDONLY | O_NONBLOCK);
    fd_smoke = open(PIPE_SMOKE, O_RDONLY | O_NONBLOCK);
    fd_servo = open(PIPE_SERVO, O_RDONLY | O_NONBLOCK);
    fd_geyser = open(PIPE_GEYSER, O_RDONLY | O_NONBLOCK);
    fd_sound = open(PIPE_SOUND, O_RDONLY | O_NONBLOCK);
    fd_status = open(PIPE_STATUS, O_RDONLY | O_NONBLOCK);
   
    // Track max file descriptor for select()
    maxfd = fd_led > fd_ac ? fd_led : fd_ac;
    maxfd = maxfd > fd_temp ? maxfd : fd_temp;
    maxfd = maxfd > fd_smoke ? maxfd : fd_smoke;
    maxfd = maxfd > fd_servo ? maxfd : fd_servo;
    maxfd = maxfd > fd_geyser ? maxfd : fd_geyser;
    maxfd = maxfd > fd_sound ? maxfd : fd_sound;
    maxfd = maxfd > fd_status ? maxfd : fd_status;
   
    while (running) {
        FD_ZERO(&readfds);
        if (fd_led >= 0) FD_SET(fd_led, &readfds);
        if (fd_ac >= 0) FD_SET(fd_ac, &readfds);
        if (fd_temp >= 0) FD_SET(fd_temp, &readfds);
        if (fd_smoke >= 0) FD_SET(fd_smoke, &readfds);
        if (fd_servo >= 0) FD_SET(fd_servo, &readfds);
        if (fd_geyser >= 0) FD_SET(fd_geyser, &readfds);
        if (fd_sound >= 0) FD_SET(fd_sound, &readfds);
        if (fd_status >= 0) FD_SET(fd_status, &readfds);
       
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
       
        int ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0) {
            // Process LED pipe
            if (fd_ac >= 0 && FD_ISSET(fd_ac, &readfds)) {
    memset(buffer, 0, sizeof(buffer));
    int n = read(fd_ac, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        trim(buffer);
        const char *new_payload = interpret_led_state(buffer) ? "on" : "off";

        // only publish if different from last time
        if (strcmp(new_payload, last_ac_payload) != 0) {
            mosquitto_publish(mosq, &req_mid, TOPIC_AC,
                              strlen(new_payload),
                              new_payload, 0, false);
            strcpy(last_ac_payload, new_payload);
            printf("? Published to %s: %s\n", TOPIC_AC, new_payload);
    }
}

           
            // Process AC pipe
            
           
            // Process temp pipe (not boolean)
            if (fd_temp >= 0 && FD_ISSET(fd_temp, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int n = read(fd_temp, buffer, sizeof(buffer)-1);
                if (n > 0) {
                    buffer[n] = '\0';
                    trim(buffer);
                    // Check if it's already JSON formatted
                    if (buffer[0] == '{') {
                        mosquitto_publish(mosq, &req_mid, TOPIC_TEMP, strlen(buffer), buffer, 0, false);
                    } else {
                        // Create a simple temperature JSON
                        char payload[128];
                        snprintf(payload, sizeof(payload), "{\"temperature\":%s}", buffer);
                        mosquitto_publish(mosq, &req_mid, TOPIC_TEMP, strlen(payload), payload, 0, false);
                    }
                    printf("? Published to %s: %s\n", TOPIC_TEMP, buffer);
                }
            }
           
            // Process smoke pipe
            if (fd_smoke >= 0 && FD_ISSET(fd_smoke, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int n = read(fd_smoke, buffer, sizeof(buffer)-1);
                if (n > 0) {
                    buffer[n] = '\0';
                    trim(buffer);
                    // Create properly formatted JSON
                    char payload[128];
                    if (atoi(buffer) != 0) {
                        snprintf(payload, sizeof(payload), "{\"smoke\":1,\"alert\":\"smoke_detected\"}");
                    } else {
                        snprintf(payload, sizeof(payload), "{\"smoke\":0}");
                    }
                    mosquitto_publish(mosq, &req_mid, TOPIC_SMOKE, strlen(payload), payload, 0, false);
                    printf("? Published to %s: %s\n", TOPIC_SMOKE, payload);
                }
            }
           
            // Process servo pipe
             if (fd_servo >= 0 && FD_ISSET(fd_servo, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int n = read(fd_servo, buffer, sizeof(buffer)-1);
                if (n > 0) {
                    buffer[n] = '\0';
                    trim(buffer);
                    // Check for special commands
                    if (strcasecmp(buffer, "open") == 0 || strcasecmp(buffer, "closed") == 0 ||
                        strcasecmp(buffer, "auto") == 0 || strcasecmp(buffer, "off") == 0 || strcasecmp(buffer, "Off") == 0) {
                            if (strcmp(buffer, last_servo_payload) != 0) {
                        mosquitto_publish(mosq, &req_mid, TOPIC_SERVO, strlen(buffer), buffer, 0, false);
                        strcpy(last_servo_payload, buffer);
                    printf("? Published to %s: %s\n", TOPIC_SERVO, buffer);

        }
                    } 
                }
            }
           
            // Process geyser pipe
            if (fd_geyser >= 0 && FD_ISSET(fd_geyser, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int n = read(fd_geyser, buffer, sizeof(buffer)-1);
                if (n > 0) {
                    buffer[n] = '\0';
                    trim(buffer);
                    // Send appropriate format for geyser control

        const char* payload;
        if (interpret_led_state(buffer)) {
            payload = "on";
        } else {
            payload = "off";
        }
                    mosquitto_publish(mosq, &req_mid, TOPIC_GEYSER, strlen(payload), payload, 0, false);
                    printf("? Published to %s: %s\n", TOPIC_GEYSER, payload);
                }
            }
           
            // Process sound pipe (if needed)
            if (fd_sound >= 0 && FD_ISSET(fd_sound, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int n = read(fd_sound, buffer, sizeof(buffer)-1);
                if (n > 0) {
                    buffer[n] = '\0';
                    trim(buffer);
                    mosquitto_publish(mosq, &req_mid, TOPIC_SOUND, strlen(buffer), buffer, 0, false);
                    printf("? Published to %s: %s\n", TOPIC_SOUND, buffer);
                }
            }
           
            // Process status pipe (if needed)
            if (fd_status >= 0 && FD_ISSET(fd_status, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int n = read(fd_status, buffer, sizeof(buffer)-1);
                if (n > 0) {
                    buffer[n] = '\0';
                    trim(buffer);
                    mosquitto_publish(mosq, &req_mid, TOPIC_STATUS, strlen(buffer), buffer, 0, false);
                    printf("? Published to %s: %s\n", TOPIC_STATUS, buffer);
                }
            }
        }
    }
   
    // Close all pipes before exit
    if (fd_led >= 0) close(fd_led);
    if (fd_ac >= 0) close(fd_ac);
    if (fd_temp >= 0) close(fd_temp);
    if (fd_smoke >= 0) close(fd_smoke);
    if (fd_servo >= 0) close(fd_servo);
    if (fd_geyser >= 0) close(fd_geyser);
    if (fd_sound >= 0) close(fd_sound);
    if (fd_status >= 0) close(fd_status);
   
    return NULL;
}

// === MQTT message handler ===


// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("\n? Shutdown signal received, cleaning up...\n");
    running = 0;
}

// === Main function ===
int main(int argc, char *argv[]) {
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
   
    // Allow broker host and port to be specified via arguments
    if (argc > 1) mqtt_host = argv[1];
    if (argc > 2) mqtt_port = atoi(argv[2]);
   
    mosquitto_lib_init();
    mosq = mosquitto_new(client_id, true, NULL);
    if (!mosq) {
        fprintf(stderr, "? Failed to create Mosquitto instance\n");
        return 1;
    }
   
    mosquitto_message_callback_set(mosq, on_message);
   
    printf("? Connecting to MQTT broker at %s:%d...\n", mqtt_host, mqtt_port);
    if (mosquitto_connect(mosq, mqtt_host, mqtt_port, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "? Unable to connect to MQTT broker at %s:%d\n", mqtt_host, mqtt_port);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
   
    // Subscribe to topics
    mosquitto_subscribe(mosq, NULL, TOPIC_TEMP, 0);
    //mosquitto_subscribe(mosq, NULL, TOPIC_LED_STATUS, 0);
    mosquitto_subscribe(mosq, NULL, TOPIC_SMOKE, 0);
    mosquitto_subscribe(mosq, NULL, TOPIC_AC, 0);
    mosquitto_subscribe(mosq, NULL, TOPIC_SERVO, 0);
    //mosquitto_subscribe(mosq, NULL, TOPIC_GEYSER, 0);
    mosquitto_subscribe(mosq, NULL, TOPIC_SOUND, 0);
    mosquitto_subscribe(mosq, NULL, TOPIC_STATUS, 0);
   
    printf("? Subscribed to all topics. Starting bi-directional communication...\n");
   
    // Start the pipe reader thread for ESP32 to MQTT communication
    pthread_create(&reader_thread, NULL, pipe_reader_thread, (void *)mosq);
   
    // Send initial status request
    //mosquitto_publish(mosq, NULL, TOPIC_STATUS, strlen("{\"request\":\"status\"}"),"{\"request\":\"status\"}", 0, false);
   
    // Run the event loop for MQTT to ESP32 communication
    while(running) {
        int rc = mosquitto_loop(mosq, 100, 1);
        if (rc != MOSQ_ERR_SUCCESS) {
            printf("?? Reconnecting to broker...\n");
            sleep(1);
            rc = mosquitto_reconnect(mosq);
            if (rc != MOSQ_ERR_SUCCESS) {
                fprintf(stderr, "? Failed to reconnect: %s\n", mosquitto_strerror(rc));
                sleep(5);
            }
        }
    }
   
    // Clean up
    printf("? Cleaning up and exiting...\n");
    pthread_join(reader_thread, NULL);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
   
    return 0;
}
