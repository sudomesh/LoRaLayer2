
#include <unistd.h>
#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define DEBUG 0
#define SHA1_LENGTH 40
#define ADDR_LENGTH 6 

typedef struct _serial {
  int (*printf)(const char*, ...);
} serial;

extern serial Serial;

// public functions
int nsleep(unsigned int secs, useconds_t usecs);
//int setTimeDistortion(float newDistortion);
//float timeDistortion();
int simulationTime(int realTime);
int getTime();
int isHashNew(uint8_t hash[SHA1_LENGTH]);
int debug_printf(const char* format, ...);
int print_err(const char* format, ...);
int setLocalAddress(char* macString);
uint8_t* localAddress();
int begin_packet();
int send_packet(char* data, uint8_t len);

// you must declare these in your router
int setup(); // called once on startup
int loop(); // called once per event loop iteration
//int packet_received(char* data, size_t len); // called when a packet is received
