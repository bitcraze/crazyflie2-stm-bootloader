#ifndef __SYSLINK_H__
#define __SYSLINK_H__

#include <stdint.h>
#include <stdbool.h>

#define SYSLINK_MTU 32

typedef struct syslinkPacket {
	union {
		struct {
			uint8_t type;
			uint8_t length;
			char data[SYSLINK_MTU];
		} __attribute__((packed));
		char raw[SYSLINK_MTU+2];
	};
} syslinkPacket_t;

bool syslinkReceive(struct syslinkPacket *packet);

bool syslinkSend(struct syslinkPacket *packet);

// Defined packet types
#define SYSLINK_RADIO_RAW      0x00
#define SYSLINK_RADIO_CHANNEL  0x01
#define SYSLINK_RADIO_DATARATE 0x02


#define SYSLINK_PM_SOURCE 0x10

#define SYSLINK_PM_ONOFF_SWITCHOFF 0x11

#define SYSLINK_PM_BATTERY_VOLTAGE 0x12
#define SYSLINK_PM_BATTERY_STATE   0x13
#define SYSLINK_PM_BATTERY_AUTOUPDATE 0x14

#define SYSLINK_OW_SCAN 0x20
#define SYSLINK_OW_READ 0x21

#endif
