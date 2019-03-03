
#include <unistd.h>
#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define DEBUG 0
#define LORA

typedef struct _serial {
  int (*printf)(const char*, ...);
} serial;

extern serial Serial;

// public functions
int debug_printf(const char* format, ...);
int setLocalAddress(char* macString);
uint8_t* localAddress();
int send_packet(char* data, uint8_t len);
