ARCH = armv7-a
MCPU = cortex-a8

TARGET = rvpb

CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-gcc
OC = arm-none-eabi-objcopy

LINKER_SCRIPT = ./feos.ld
MAP_FILE = build/feos.map

ASM_SRCS = $(wildcard boot/*.S)
ASM_OBJS = $(patsubst boot/%.S, build/%.os, $(ASM_SRCS))

VPATH = boot \
        hal/$(TARGET)	\
		  library	\
		  kernel

C_SRCS  = $(notdir $(wildcard boot/*.c))
C_SRCS += $(notdir $(wildcard hal/$(TARGET)/*.c))
C_SRCS += $(notdir $(wildcard library/*.c))
C_SRCS += $(notdir $(wildcard kernel/*.c))
C_OBJS = $(patsubst %.c, build/%.o, $(C_SRCS))

INC_DIRS  = -I include \
            -I hal		\
				-I hal/$(TARGET)	\
				-I library	\
				-I kernel

CFLAGS = -c -g -std=c11 -mthumb-interwork

LDFLAGS = -nostartfiles -nostdlib -nodefaultlibs -static -lgcc

feos = build/feos.axf
feos_bin = build/feos.bin

.PHONY: all clean run debug gdb

all: $(feos)

clean:
	@rm -rf build
	
run: $(feos)
	qemu-system-arm -M realview-pb-a8 -kernel $(feos) -nographic #-nographic 옵션으로 종료불가, ctrl+A -> 마우스 터미널 외 위치한 후 -> x
	
debug: $(feos)
	qemu-system-arm -M realview-pb-a8 -kernel $(feos) -S -gdb tcp::1234,ipv4
	
gdb:
	arm-none-eabi-gdb

kill:
	kill -9 `ps aux | grep 'qemu' | awk 'NR==1{print $$2}'`
	
$(feos): $(ASM_OBJS) $(C_OBJS) $(LINKER_SCRIPT)
	$(LD) -n -T $(LINKER_SCRIPT) -o $(feos) $(ASM_OBJS) $(C_OBJS) -Wl,-Map=$(MAP_FILE) $(LDFLAGS)
	$(OC) -O binary $(feos) $(feos_bin)
	
build/%.os: %.S
	mkdir -p $(shell dirname $@)
	$(CC) -march=$(ARCH) -mcpu=$(MCPU) $(INC_DIRS) $(CFLAGS) -o $@ $<
	
build/%.o: %.c
	mkdir -p $(shell dirname $@)
	$(CC) -march=$(ARCH) -mcpu=$(MCPU) $(INC_DIRS) $(CFLAGS) -o $@ $<
