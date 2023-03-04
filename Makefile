#platform := qemu
platform := k210
V := @
T := .
S := src
O := objs
FW := bootloader
QEMU := /opt/qemu/bin/qemu-system-riscv64
GCCPREFIX := riscv64-unknown-elf-

CC := $(GCCPREFIX)gcc
LD := $(GCCPREFIX)ld

CP := cp
RM := rm

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -g
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I./include
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

ifeq ($(platform), qemu)
CFLAGS += -D QEMU
endif
ifeq ($(platform), qemu)
CFLAGS += -D K210
endif

LDFLAGS	:= -nostdlib

OBJCOPY := $(GCCPREFIX)objcopy
OBJDUMP := $(GCCPREFIX)objdump
GDB := $(GCCPREFIX)gdb

kernel := $T/kernel
bin := $T/kernel.bin
k210_bin := $T/k210.bin
opensbi_elf := $(FW)/fw_jump.elf
opensbi_bin := $(FW)/fw_jump.bin

ifeq ($(platform), k210)
rustsbi_elf := $(FW)/rustsbi-k210
rustsbi_bin := $(FW)/rustsbi-k210.bin
#rustsbi_bin := $(FW)/sbi-k210.bin
else
rustsbi_elf := $(FW)/rustsbi-qemu
rustsbi_bin := $(FW)/rustsbi-qemu.bin
endif



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
  $S/panic.o \
  $S/syscall.o \
  $S/max_heap.o \
  $S/bitmap_buddy.o \
  $S/mm_heap_allocator.o \
  $S/mm_frame_allocator.o \
  $S/mm_page_table.o \
  $S/mm_memoryset.o \
  $S/list.o \
  $S/sync_spinlock.o \
  $S/sync_sem.o \
  $S/d_utils.o \
  $S/d_sysctl.o \
  $S/d_spi.o \
  $S/d_gpiohs.o \
  $S/d_fpioa.o \
  $S/d_dmac.o \
  $S/d_sdcard.o \
  $S/d_clint.o \
  $S/d_rtc.o \
  $S/sleep.o \
  $S/tk_proc.o \
  $S/tk_manage.o \
  $S/tk_pid.o \
  $S/tk_switch.o \
  $S/tk_task.o \
  $S/fs_disk.o \
  $S/fs_buf.o \
  $S/fs_fat32.o \
  $S/fs_file.o \
  $S/fs_pipe.o


#SRC1 := $(wildcard $S/*.c)
#SRC2 := $(wildcard $S/*.S)
#OBJS1 := $(addprefix $O/, $(notdir $(patsubst %.c,%.o,$(SRC1))))
#OBJS2 := $(addprefix $O/, $(notdir $(patsubst %.S,%.o,$(SRC2)))) 

ifeq ($(platform), qemu)
linker := $S/linker.ld
endif
ifeq ($(platform), k210)
linker = $S/k210.ld
endif



kernel: $(OBJS) $(linker)
	if [ ! -d "./target" ]; then mkdir target; fi
	@$(LD) $(LDFLAGS) -T $(linker) -o $(kernel) $(OBJS)
	$(OBJCOPY) $(kernel) --strip-all -O binary $(bin)
	@dd if=$(bin) of=$(rustsbi_bin) bs=128k seek=1
	@$(CP) $(rustsbi_bin) $(T)/$(k210_bin)
	@$(RM) $(bin)
	@$(OBJDUMP) -D -b binary -m riscv $(rustsbi_bin) > $T/k210.asm




all: kernel


qemu: $(bin)
	$(V)$(QEMU) \
		-machine virt \
		-nographic \
		-bios $(rustsbi_bin) \
		-device loader,file=$(bin),addr=0x80200000

debug: $(bin)
	@tmux new-session -d \
		"$(QEMU) -machine virt -nographic -bios $(rustsbi_elf) -device loader,file=$(bin),addr=0x80200000 \
		-s -S" && \
		tmux split-window -h "$(GDB) -ex 'file $(kernel)' -ex 'set arch riscv:rv64' -ex 'target remote localhost:1234'" && \
		tmux -2 attach-session -d



k210-serialport := /dev/ttyUSB0
ifndef CPUS
CPUS := 2
endif



k210: all
	@sudo chmod 777 $(k210-serialport)
	@python3 ./tools/kflash.py -p $(k210-serialport) -b 1500000 -t $(rustsbi_bin)
	#python3 -m serial.tools.miniterm --eol LF --dtr 0 --rts 0 --filter direct $(k210-serialport) 115200










clean:
	-rm -f $S/*.o $S/*.d \
  ./k210* \
