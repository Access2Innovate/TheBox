#include <Arduino.h>

#define DAT_PIN 11            // DO NOT CHANGE UNLESS YOU KNOW WHAT YOU'RE DOING
#define DAT_PIN_PORT_ID 0x08  // DO NOT CHANGE UNLESS YOU KNOW WHAT YOU'RE DOING
#define CLK_PIN 13            // DO NOT CHANGE UNLESS YOU KNOW WHAT YOU'RE DOING
#define CLK_PIN_PORT_ID 0x20  // DO NOT CHANGE UNLESS YOU KNOW WHAT YOU'RE DOING

void send_long(unsigned long l);	// Send 32 bits through pins 11 and 13
void led_write(unsigned long colour);	// Set the colour of the next LED
void strip_update(int n, unsigned long* &colours);	// Sync entire strip of N length with array of colours
void strip_fill(int n, unsigned long colour);	// Fill entire strip of N length with same colour
void strip_init();	// Initialize pins for transmission