#include <LoRaLayer2.h>

/* Fifo Buffer Class
*/
packetBuffer::packetBuffer() :
  head(0),
  tail(0)
{
};

// reads a packet from buffer
BufferEntry packetBuffer::read(){
    BufferEntry entry;
    if (head == tail){
        // if buffer empty, return empty entry
        BufferEntry entry;
        memset(&entry, 0, sizeof(entry));
        entry.length = 0;
    }
    else{
        // otherwise return entry from tail
        tail = (tail + 1) % BUFFERSIZE;
        memcpy(&entry, &buffer[tail], sizeof(entry));
        // clear data stored in tail of buffer
        memset(&buffer[tail], 0, sizeof(buffer[tail]));
    }
    return entry;
}

// writes a packet to buffer
int packetBuffer::write(BufferEntry entry) {
    int ret = BUFFERSIZE;
    // if full, return the size of the buffer
    if (((head + 1) % BUFFERSIZE) != tail){
        head = (head + 1) % BUFFERSIZE;
        // clear any previous data stored in buffer
        memset(&buffer[head], 0, sizeof(buffer[head]));
        // copy new data into buffer
        buffer[head] = entry;
        ret = head-tail; // and return the packet's place in line
    }
    // if full, return the size of the buffer
    return ret;
}
