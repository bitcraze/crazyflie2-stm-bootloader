/* Crtp packet format header */
#ifndef __CTRP_H__
#define __CRTP_H__

#include <stdint.h>

#define CRTP_MAX_DATA_SIZE 31

typedef struct crtpPacket_s {
  union {
    struct {
      union {
        uint8_t header;
        struct {
          uint8_t channel:2;
          uint8_t reserved:2;
          uint8_t port:4;
        };
      };
      uint8_t data[CRTP_MAX_DATA_SIZE];
    } __attribute__((packed));
    char raw[CRTP_MAX_DATA_SIZE +1];
  };
  uint8_t datalen;
} CrtpPacket;

#endif
