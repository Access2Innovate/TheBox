#include "LED_Strip.h"

void send_long(unsigned long l) {
  char n = 32;

  while (n--) {
    PORTB &= ~CLK_PIN_PORT_ID;
    PORTB = (PORTB & ~DAT_PIN_PORT_ID) | (l << 3 & DAT_PIN_PORT_ID);	// Ensure bit shift is correct if pins change
    PORTB |= CLK_PIN_PORT_ID;
    l >>= 1;
  }
}

void led_write(unsigned long colour) {
  send_long((colour << 8) | 0xFF);
}

void strip_update(int n, unsigned long* &colours) {
  send_long(0x00000000);

  while (n--) {
    led_write(colours[n]);
  }

  send_long(0xFFFFFFFF);
}

void strip_fill(int n, unsigned long colour) {
  send_long(0x00000000);

  while (n--) {
    led_write(colour);
  }

  send_long(0xFFFFFFFF);
}

void strip_init() {
  pinMode(DAT_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
}