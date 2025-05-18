#ifndef PS2_H
#define PS2_H

#define PS2_DATA 0x60
#define PS2_CMD  0x64

#define PS2_STATUS_OUT 0x01
#define PS2_STATUS_IN  0x02

#define PS2_CMD_READ_CONFIG  0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_DISABLE1     0xAD
#define PS2_CMD_ENABLE1      0xAE
#define PS2_CMD_DISABLE2     0xA7
#define PS2_CMD_ENABLE2      0xA8
#define PS2_CMD_WRITE_PORT2  0xD4

#define PS2_CONFIG_PORT1_INT 0x01
#define PS2_CONFIG_PORT2_INT 0x02
#define PS2_CONFIG_PORT1_CLK 0x10
#define PS2_CONFIG_PORT2_CLK 0x20

#define MOUSE_CMD_DEFAULT 0xF6
#define MOUSE_CMD_ENABLE  0xF4

void Ps2Initialize();

#endif
