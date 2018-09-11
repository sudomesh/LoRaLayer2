
#define STDIN 0
#define STDOUT 1

struct serial {
  int (*printf)(const char*, ...);
};

struct serial Serial;

int print_err(const char* format, ...);

int send_packet(char* data, size_t len);


// you must declare these in your router
int setup(); // called once on startup
int loop(); // called once per event loop iteration
int packet_received(char* data, size_t len); // called when a packet is received
