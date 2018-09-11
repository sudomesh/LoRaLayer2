#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include "base.h"



int print_err(const char* format, ...) {
  int ret;

  va_list args;
  va_start(args, format);
  ret = vfprintf(stderr, format, args);
  va_end(args);
  return ret;
}

int send_packet(char* data, size_t len) {

  ssize_t written = 0;
  ssize_t ret;

  if(!len) {
    len = strlen(data);
  }

  if(len > 256) {
    fprintf(stderr, "Attempted to send packet larger than 256 bytes\n");
    return -1;
  }

  while(ret < 1) {
    ret = write(STDOUT, (void*) &len, 1);
    if(ret < 0) {
      return ret;
    }
  }
  while(written < len) {
    ret = write(STDOUT, (void*) data, len);
    if(ret < 0) {
      return ret;
    }
    written += ret;
  }

  return 0;
}

int main(int argc, char **argv) {

  char buffer[257];
  struct timeval tv;
  fd_set fds;
  int flags;
  ssize_t ret;
  ssize_t len = 0;
  ssize_t got;

  Serial.printf = &printf;

  flags = fcntl(STDIN, F_GETFL, 0);
  if(flags == -1) {
    perror("Failed to get socket flags\n");
    return 1;
  }

  flags = flags | O_NONBLOCK;
  if(ret = fcntl(STDIN, F_SETFL, flags)) {
    perror("Failed to set non-blocking mode\n");
    return 1;
  }

  if(ret = setup()) {
    return ret;
  }

  while(1) {
    FD_ZERO(&fds);
    FD_SET(STDIN, &fds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(STDIN + 1, &fds, NULL, NULL, &tv);
    if(ret < 0) {
      perror("select() failed");
      return 1;
    } else if(ret && FD_ISSET(STDIN, &fds)) {

      if(!len) {
        // receive the length of the incoming packet
        ret = read(STDIN, &len, 1);
        if(ret < 0) {
          perror("receiving length of incoming packet failed");
          return 1;
        }
        len += 1;

        got = 0;
      }

      while(got < len) {
        ret = read(STDIN, (void*) buffer+got, len-got);
        if(ret < 0) {
          if(errno == EAGAIN) { // we need to wait for more data
            break;
          }
          perror("receiving incoming data failed");
          return 1;
        }
        got += ret;
      }

      if(got < len) {
        continue;
      }

      if(ret = packet_received(buffer, len)) {
        return ret;
      }
      len = 0;
    }
    if(ret = loop()) {
      return ret;
    }
  }

  return 0;
}
