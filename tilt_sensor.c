#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>

#define TILT_PIN 0  // GPIO17

int main() {
    if (wiringPiSetup() == -1) {
        printf("WiringPi setup failed\n");
        return 1;
    }

    pinMode(TILT_PIN, INPUT);

    printf("Reading tilt sensor 10 times...\n");

    for (int i = 0; i < 10; i++) {
        int tilt = digitalRead(TILT_PIN);
        printf("Read %d: %d\n", i + 1, tilt);
        sleep(1);
    }

    return 0;
}
