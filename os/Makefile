platform := qemu
V := @
T := target
S := src
O := objs
FW := ../bootloader
QEMU := qemu-system-riscv64
GCCPREFIX := riscv64-unknown-elf-

CC := $(GCCPREFIX)gcc
LD := $(GCCPREFIX)ld

CFLAGS := -mcmodel=medany -std=gnu99 -Wno-unused -Werror
CFLAGS += -fno-builtin -nostdinc
CFLAGS += -nostdlib -g
CFLAGS += -I./include

LDFLAGS	:= -nostdlib

OBJCOPY := $(GCCPREFIX)objcopy
OBJDUMP := $(GCCPREFIX)objdump
GDB := $(GCCPREFIX)gdb

kernel := $T/kernel
bin := $T/kernel.bin
opensbi_elf := $(FW)/fw_jump.elf
opensbi_bin := $(FW)/fw_jump.bin




OJS =
ifeq ($(platform), k210)
OBJS += $S/entry_k210.o
else
OBJS += $S/entry.o
endif

OBJS += \
  $S/main.o \
  $S/sbi.o \
  $S/console.o \
  $S/stdio.o \
  $S/string.o \
  $S/timer.o \
  $S/exception.o \
  $S/exception_entry.o \

#SRC1 := $(wildcard $S/*.c)
#SRC2 := $(wildcard $S/*.S)
#OBJS1 := $(addprefix $O/, $(notdir $(patsubst %.c,%.o,$(SRC1))))
#OBJS2 := $(addprefix $O/, $(notdir $(patsubst %.S,%.o,$(SRC2))))

linker := $S/linker.ld


kernel: $(OBJS) $(linker)
	if [ ! -d "./target" ]; then mkdir target; fi
	@$(LD) $(LDFLAGS) -T $(linker) -o $T/kernel $(OBJS)


$(bin): kernel
	$(OBJCOPY) $(kernel) --strip-all -O binary $@


qemu: $(bin)
	$(V)$(QEMU) \
		-machine virt \
		-nographic \
		-bios $(opensbi_bin) \
		-device loader,file=$(bin),addr=0x80200000

debug: $(bin)
	@tmux new-session -d \
		"$(QEMU) -machine virt -nographic -bios $(opensbi_elf) -device loader,file=$(bin),addr=0x80200000 \
		-s -S" && \
		tmux split-window -h "$(GDB) -ex 'file $(kernel)' -ex 'set arch riscv:rv64' -ex 'target remote localhost:1234'" && \
		tmux -2 attach-session -d


clean:
	-rm -i $S/*.o \
	$T/*
