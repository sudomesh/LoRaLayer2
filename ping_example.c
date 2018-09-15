#include <stdio.h>
#include "base.h"

int setup() {

  Serial.printf("initialized\n");

  return 0;
}

int count = 0;

int loop() {

  size_t send_len;
  char test[256];
  Serial.printf("--- looping\n");

  send_len = snprintf(test, 256, "test packet %d \0", count);
  send_packet(test , send_len);

  count++;
  // see README.md for explanation of nsleep
  nsleep(5, 0);
}


int packet_received(char* data, size_t len) {

  size_t send_len;
  char foo[256];

  data[len] = '\0';
  //Serial.printf("received %d bytes: %s\n", len, data);

  //send_len = snprintf(foo, 256, "Got a packet of %d bytes", (int) len);
  //send_packet(foo, send_len);
}

