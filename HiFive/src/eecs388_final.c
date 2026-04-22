#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "eecs388_lib.h"

void ser_printf(const char *format, ...) {
    char buffer[128];

    va_list args;

	va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    const char *p = buffer;
    while (*p) {
        ser_write(1, *p++);
    }

    ser_write(1, '\n');
    ser_write(1, '\r');
}

void auto_brake(int devid)
{
    uint16_t dist = 0;
    if (ser_read(devid) == 'Y' && ser_read(devid) == 'Y') {

        uint8_t low = ser_read(devid);
        uint8_t high = ser_read(devid);
        
        // Read unused variables
        uint16_t str_l = ser_read(devid); 
        uint16_t str_h = ser_read(devid); 
        uint16_t rsvd = ser_read(devid); 
        uint16_t qlty = ser_read(devid); 
        uint16_t chksm = ser_read(devid);

        dist = (high << 8) | low;
        printf("Distance: %d\n", dist);

        if (dist > 200) {
            // Green
            gpio_write(GREEN_LED, ON);
            gpio_write(RED_LED, OFF);
        }
        else if (dist > 100) {
            // Yellow
            gpio_write(GREEN_LED, ON);
            gpio_write(RED_LED, ON);
        }
        else if (dist > 60) {
            // Red
            gpio_write(GREEN_LED, OFF);
            gpio_write(RED_LED, ON);
        }
        else {
            // Flashing Red
            gpio_write(GREEN_LED, OFF);
            gpio_write(RED_LED, ON);
            printf("Brake\n");
            delay(100);
            gpio_write(RED_LED, OFF);
            delay(100);
        }
    }
}

// Task 3
int read_from_pi(int devid)
{
    char buf[32];
    int i = 0;
    char c;

    // Read characters until newline or buffer full
    while (i < (int)(sizeof(buf) - 1)) {
        c = ser_read(devid);
        if (c == '\n' || c == '\r') {
            break;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';

    int angle = 0;
    float f = 0.0f;
    if (sscanf(buf, "%f", &f) == 1) {
        angle = (int)f;
    }
    return angle;
}

// Task 4
void steering(int gpio, int pos)
{
    // Clamp angle to [0, 180]
    if (pos < 0)   pos = 0;
    if (pos > 180) pos = 180;

    // pulse width 0.5ms at 0 deg and 2.5ms at 180 deg
    int pulse_us = 500 + (pos * 2000 / 180);

    // total period is 20ms
    int low_us = 20000 - pulse_us;

    gpio_write(gpio, ON);
    delay_usec(pulse_us);
    gpio_write(gpio, OFF);
    delay_usec(low_us);
}


int main()
{
    // UART setup
    ser_setup(0); // Lidar
    ser_setup(1); // Raspberry Pi

    int pi_to_hifive = 1;
    int lidar_to_hifive = 0;

    printf("\nUsing UART %d for Pi -> HiFive", pi_to_hifive);
    printf("\nUsing UART %d for Lidar -> HiFive", lidar_to_hifive);

    // GPIO setup
    gpio_mode(PIN_19, OUTPUT); // Servo
    gpio_mode(RED_LED, OUTPUT);
    gpio_mode(BLUE_LED, OUTPUT);
    gpio_mode(GREEN_LED, OUTPUT);

    printf("\nSetup completed.\n");
    printf("Begin main loop.\n");

    while (1) {

        // Task 1 & 2
        auto_brake(lidar_to_hifive);

        // Task 3
        // int angle = read_from_pi(pi_to_hifive);
        // printf("Angle: %d\n", angle);

        // Task 4
    }

    return 0;
}
