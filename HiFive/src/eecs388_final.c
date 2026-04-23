#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "eecs388_lib.h"

#define LED_GREEN        0
#define LED_YELLOW       1
#define LED_RED          2
#define LED_FLASHING_RED 3

static int led_state = LED_GREEN;
static int flash_on  = 0;

// task 1 & 2
int auto_brake(int devid)
{
    while (ser_isready(devid) & 0x2) {
        uint8_t b1 = (uint8_t)ser_read(devid);
        if (b1 != 'Y') continue;

        uint8_t b2 = (uint8_t)ser_read(devid);
        if (b2 != 'Y') continue;

        uint8_t low  = (uint8_t)ser_read(devid);
        uint8_t high = (uint8_t)ser_read(devid);
        ser_read(devid);
        ser_read(devid);
        ser_read(devid);
        ser_read(devid);
        ser_read(devid);

        uint16_t dist = (high << 8) | low;

        if (dist > 1200) continue;
        printf("Distance: %d\n", dist);
        if (dist > 200)      led_state = LED_GREEN;
        else if (dist > 100) led_state = LED_YELLOW;
        else if (dist > 60)  led_state = LED_RED;
        else if (dist > 0)   led_state = LED_FLASHING_RED;
        return 1;
    }
    return 0;
}

// task 3
int read_from_pi(int devid)
{
    static char buf[32];
    static int idx = 0;

    if (!(ser_isready(devid) & 0x2)) return -1;

    int c = (unsigned char)ser_read(devid);

    if (c == '\r') return -1;

    if (c == '\n') {
        buf[idx] = '\0';
        idx = 0;

        if (buf[0] == '\0') return -1;

        int angle;
        if (sscanf(buf, "%d", &angle) == 1) {
            if (angle < -360 || angle > 360) return -1;
            return angle;
        }
        return -1;
    }

    if ((c >= '0' && c <= '9') || (c == '-' && idx == 0)) {
        if (idx < 31) {
            buf[idx++] = (char)c;
        }
    } else {
        idx = 0;
    }

    return -1;
}

// task 4
void steering(int gpio, int pos)
{
    if (pos < 0)   pos = 0;
    if (pos > 180) pos = 180;

    uint32_t pulse_us = 544 + ((uint32_t)pos * (2400 - 544) / 180);
    uint32_t low_us   = 20000 - pulse_us;

    gpio_write(gpio, ON);
    delay_usec(pulse_us);
    gpio_write(gpio, OFF);
    delay_usec(low_us);
}

// function to simplify led writing
void update_leds()
{
    switch (led_state) {
        case LED_GREEN:
            gpio_write(GREEN_LED, ON);
            gpio_write(RED_LED, OFF);
            break;
        case LED_YELLOW:
            gpio_write(GREEN_LED, ON);
            gpio_write(RED_LED, ON);
            break;
        case LED_RED:
            gpio_write(GREEN_LED, OFF);
            gpio_write(RED_LED, ON);
            break;
        case LED_FLASHING_RED:
            gpio_write(GREEN_LED, OFF);
            gpio_write(RED_LED, flash_on ? ON : OFF);
            flash_on = !flash_on;
            break;
    }
}

// main
int main()
{
    ser_setup(0);
    ser_setup(1);

    int lidar_uart = 0;
    int pi_uart    = 1;
    int servo_pin  = PIN_19;

    gpio_mode(servo_pin, OUTPUT);
    gpio_mode(RED_LED, OUTPUT);
    gpio_mode(BLUE_LED, OUTPUT);
    gpio_mode(GREEN_LED, OUTPUT);

    printf("System Initialized\n");

    int last_angle = 90;
    uint64_t last_flash_time = get_cycles();
    uint64_t cycles_100ms = (uint64_t)32768 * 100 / 1000;

    while (1) {
        // Task 1 & 2: drain all available LiDAR bytes
        auto_brake(lidar_uart);

        // Task 3: read Pi angle if available
        if (ser_isready(pi_uart) & 0x2) {
            int angle = read_from_pi(pi_uart);
            if (angle != -1) {
                if (angle < 0)        last_angle = 0;
                else if (angle > 180) last_angle = 180;
                else                  last_angle = angle;
                printf("Angle: %d\n", last_angle);
            }
        }

        // Task 4: one PWM pulse per loop iteration
        steering(servo_pin, last_angle);

        // Update LEDs every 100ms without blocking
        uint64_t now = get_cycles();
        if ((now - last_flash_time) >= cycles_100ms) {
            update_leds();
            last_flash_time = now;
        }
    }
    return 0;
}
