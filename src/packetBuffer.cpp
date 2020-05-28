#include <LoRaLayer2.h>

/* Fifo Buffer Class
*/
packetBuffer::packetBuffer()
{
    head = 0;
    tail = 0;
}

// reads a packet from buffer
char* packetBuffer::read(){
    // clear tmp_data of any previous packet data
    memset(&tmp_data, 0, sizeof(tmp_data));
    if (head != tail){
      tail = (tail + 1) % BUFFERSIZE;
      // copy contents of buffer tail to tmp_data
      memcpy(&tmp_data, &buffer[tail][0], sizeof(tmp_data));
    }
    return tmp_data;
}

// writes a packet to buffer
int packetBuffer::write(char data[MAX_PACKET_SIZE], size_t len) {
    int ret = BUFFERSIZE;
    // if full, return the size of the buffer
    if (((head + 1) % BUFFERSIZE) != tail){
        head = (head + 1) % BUFFERSIZE;
        // clear any previous data stored in buffer
        memset(&buffer[head][0], 0, sizeof(buffer[head][0]));
        // copy new data into buffer
        memcpy(&buffer[head][0], &data[0], len);
        ret = head-tail; // and return the packet's place in line
    }
    return ret;
}
