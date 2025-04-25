#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTClient.h"

#define ADDRESS     "ssl://52b1c31363bc477e8460c34c2532a968.s1.eu.hivemq.cloud:8883"
#define CLIENTID    "mqtt_dht_client"
#define TOPIC       "dht11/temperature"
#define QOS         1
#define TIMEOUT     10000L
#define MQTT_USER   "hithesh"
#define MQTT_PASS   "Qwerty@123"
#define CA_CERT     "hivemq_ca.crt"
#define PIPE_PATH   "/tmp/mqtt_pipe"

volatile MQTTClient_deliveryToken deliveredtoken;

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    printf("? Message arrived on topic %s: %.*s\n", topicName, message->payloadlen, (char*)message->payload);

    // ? Send to Python via named pipe
    FILE *pipe = fopen(PIPE_PATH, "w");
    if (pipe) {
        fprintf(pipe, "%.*s\n", message->payloadlen, (char*)message->payload);
        fclose(pipe);
        printf("? Sent to Python via pipe: %.*s\n", message->payloadlen, (char*)message->payload);
    } else {
        perror("? Failed to open pipe");
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int main() {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    conn_opts.username = MQTT_USER;
    conn_opts.password = MQTT_PASS;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    // ? Setup SSL certificate for HiveMQ Cloud
    ssl_opts.trustStore = CA_CERT;
    ssl_opts.enableServerCertAuth = 1;
    conn_opts.ssl = &ssl_opts;

    MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("? Failed to connect, return code %d\n", rc);
        return -1;
    }

    printf("? Connected to HiveMQ Cloud\n");

    MQTTClient_subscribe(client, TOPIC, QOS);

    // ? Keep the app alive
    while (1) {
        sleep(1);
    }

    // Cleanup (never reached in this version)
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return 0;
}
