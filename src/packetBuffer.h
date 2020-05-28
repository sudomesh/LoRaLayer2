#ifndef PACKETBUFFER_H
#define PACKETBUFFER_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
//#include <LoRaLayer2.h>
#define BUFFERSIZE 16
#define MAX_PACKET_SIZE 255

class packetBuffer {
  public:
    // use pointer to char instead? and then copy that raw data into a packet
    char* read();
    int write(char data[MAX_PACKET_SIZE], size_t len);
    packetBuffer();
  private:
    char buffer[BUFFERSIZE][MAX_PACKET_SIZE];
    char tmp_data[MAX_PACKET_SIZE];
    int head;
    int tail;
};
#endif
