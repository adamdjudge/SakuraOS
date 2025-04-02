#include <kernel.h>
#include <ps2.h>
#include <x86.h>
#include <vesa.h>
#include <console.h>

static int8_t mouseData[3];
static uint8_t cycle = 0;

static int newX = 0, newY = 0, oldX, oldY;
static bool ready = false;

extern void GuiMoveMouse();

static uint8_t readData()
{
    while ((in_byte(PS2_CMD) & PS2_STATUS_OUT) == 0) {}
    return in_byte(PS2_DATA);

    int delay = 10000;
    while (delay > 0)
        delay--;
}

static void writeCommand(uint8_t cmd)
{
    while ((in_byte(PS2_CMD) & PS2_STATUS_IN) == 1) {}
    out_byte_wait(PS2_CMD, cmd);

    int delay = 10000;
    while (delay > 0)
        delay--;
}

static void writeData(uint8_t data)
{
    while ((in_byte(PS2_CMD) & PS2_STATUS_IN) == 1) {}
    out_byte_wait(PS2_DATA, data);

    int delay = 10000;
    while (delay > 0)
        delay--;
}

void Ps2Initialize()
{
    uint8_t config;

    writeCommand(PS2_CMD_DISABLE1);
    writeCommand(PS2_CMD_DISABLE2);

    in_byte(PS2_DATA); // Flush out buffer
    
    writeCommand(PS2_CMD_READ_CONFIG);
    config = readData() | PS2_CONFIG_PORT1_INT | PS2_CONFIG_PORT2_INT;
    writeCommand(PS2_CMD_WRITE_CONFIG);
    writeData(config);

    writeCommand(PS2_CMD_ENABLE1);
    writeCommand(PS2_CMD_ENABLE2);

    writeCommand(PS2_CMD_WRITE_PORT2);
    writeData(MOUSE_CMD_DEFAULT);
    readData(); // Acknowledge

    writeCommand(PS2_CMD_WRITE_PORT2);
    writeData(MOUSE_CMD_ENABLE);
    readData(); // Acknowledge
    
    ready = true;
}

void Ps2HandleMouseInterrupt()
{
    uint8_t data;

    if (!ready)
        return;

    data = in_byte(PS2_DATA);
    if (cycle == 0 && !(data & 0x8)) {
        return; // Re-sync with stream
    }
    mouseData[cycle++] = data;

    if (cycle == 3) {
        cycle = 0;
        if (mouseData[0] & 0xC0) {
            return; // Discard overflows
        }

        oldX = newX;
        oldY = newY;
        newX = oldX + mouseData[1];
        newY = oldY - mouseData[2];
        if (newX < 0) newX = 0;
        if (newX >= g_ModeInfo.width) newX = g_ModeInfo.width - 1;
        if (newY < 0) newY = 0;
        if (newY >= g_ModeInfo.height) newY = g_ModeInfo.height - 1;

        //if (mouseData[0] & 1)
        if (mouseData[1] | mouseData[2])
            GuiMoveMouse(newX, newY);
    }
}
