ARCH_PATH = arch/x86/
include $(ARCH_PATH)/arch.mk

OBJS = $(addprefix $(ARCH_PATH), $(ARCH_OBJS))
OBJS += debug.o

LINKER_SCRIPT = $(ARCH_PATH)$(ARCH_LINKER_SCRIPT)

TARGET = kernel.elf

all: $(TARGET)

$(TARGET): $(LINKER_SCRIPT) $(OBJS)
	$(LD) -T $(LINKER_SCRIPT) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

run: $(TARGET)
	qemu-system-i386 -serial stdio -kernel $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
