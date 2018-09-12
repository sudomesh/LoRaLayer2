#include <stdio.h>
#include "base.h"

int setup() {

  Serial.printf("initialized\n");

  return 0;
}


int loop() {
  //  Serial.printf("--- looping\n");

  // see README.md for explanation of nsleep
  nsleep(2, 0);
}


int packet_received(char* data, size_t len) {

  size_t send_len;
  char foo[256];

  data[len] = '\0';
  Serial.printf("received %d bytes: %s\n", len, data);

  send_len = snprintf(foo, 256, "Got a packet of %d bytes", (int) len);
  send_packet(foo, send_len);
}

