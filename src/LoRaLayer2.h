#include <unistd.h>
#include <stdint.h>

//#define DEBUG 0
#define HEADER_LENGTH 16
#define PACKET_LENGTH 256
#define SHA1_LENGTH 40
#define ADDR_LENGTH 6 
#define MAX_ROUTES_PER_PACKET 30
#define ASYNC_TX 1;
#define DEFAULT_TTL 30;

struct Metadata {
    uint8_t rssi;
    uint8_t snr;
    uint8_t randomness;
};

struct Packet {
    uint8_t ttl;
    uint8_t totalLength;
    uint8_t source[ADDR_LENGTH];
    uint8_t destination[ADDR_LENGTH];
    uint8_t sequence;
    uint8_t type;
    uint8_t data[240];
};

struct RoutedMessage {
    uint8_t nextHop[6];
    uint8_t data[234];
};

struct NeighborTableEntry{
    uint8_t address[ADDR_LENGTH];
    uint8_t lastReceived;
    uint8_t packet_success;
    uint8_t metric;
};

struct RoutingTableEntry{
    uint8_t destination[ADDR_LENGTH];
    uint8_t nextHop[ADDR_LENGTH];
    uint8_t distance;
    uint8_t lastReceived;
    uint8_t metric;
};

class LL2Class {
public:
    LL2Class();
    uint8_t messageCount();
    int pushToOutBuffer(struct Packet packet);
    int sendToLayer2(uint8_t dest[6], uint8_t type, uint8_t data[240], uint8_t dataLength);
    struct Packet popFromChatBuffer();
    struct Packet buildPacket(uint8_t ttl, uint8_t src[6], uint8_t dest[6], uint8_t sequence, uint8_t type, uint8_t data[240], uint8_t dataLength);
    void printAddress(uint8_t address[ADDR_LENGTH]);
    void debug_printAddress(uint8_t address[ADDR_LENGTH]);
    int packetReceived(char* data, size_t len);
    int daemon();
    int getRouteEntry();
    long setInterval(long interval);
    int init();

private:
    int sendToLayer1(struct Packet packet);
    struct Packet popFromOutBuffer();
    int pushToChatBuffer(struct Packet packet);
    int pushToInBuffer(struct Packet packet);
    struct Packet popFromInBuffer();
    void printMetadata(struct Metadata metadata);
    void printPacketInfo(struct Packet packet);
    void printNeighborTable();
    void printRoutingTable();
    long transmitHello(long interval, long lastTime);
    long transmitToRoute(long interval, long lastTime, int dest);
    struct Packet buildRoutingPacket();
    void checkOutBuffer();
    void checkInBuffer();

    uint8_t calculatePacketLoss(int entry, uint8_t sequence);
    uint8_t calculateMetric(int entry, uint8_t sequence, struct Metadata metadata);
    int checkNeighborTable(struct NeighborTableEntry neighbor);
    int checkRoutingTable(struct RoutingTableEntry route);
    int updateNeighborTable(struct NeighborTableEntry neighbor, int entry);
    int updateRouteTable(struct RoutingTableEntry route, int entry);
    int selectRoute(struct Packet packet);
    void retransmitRoutedPacket(struct Packet packet, struct RoutingTableEntry route);
    int parseHelloPacket(struct Packet packet, struct Metadata metadata);
    int parseRoutingPacket(struct Packet packet, struct Metadata metadata);
    void parseChatPacket(struct Packet packet);

private:
    uint8_t _messageCount;
    float _packetSuccessWeight;
    float _randomMetricWeight;
    struct Packet _outBuffer[8];
    int _outBufferEntry;
    struct Packet _inBuffer[8];
    int _inBufferEntry;
    struct Packet _chatBuffer[8];
    int _chatBufferEntry;
    struct NeighborTableEntry _neighborTable[255];
    int _neighborEntry;
    struct RoutingTableEntry _routeTable[255];
    int _routeEntry;
    long _startTime;
    long _lastRoutingTime;
    int _routingInterval;
    int _disableRoutingPackets;
};

extern LL2Class LL2;
