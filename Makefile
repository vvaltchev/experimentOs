
# Master Makefile of the project

export ARCH = i386

ifeq ($(strip $(ARCH)),i386)
export ARCH_CFLAGS = -m32 -march=i686
export ELF_LD_FORMAT = elf_i386
export ELF_NASM_FORMAT = elf32
endif

ifeq ($(strip $(ARCH)),x86_64)
export ARCH_CFLAGS = -m64 -march=amd64
export ELF_LD_FORMAT = elf_x86_64
export ELF_NASM_FORMAT = elf64
endif

# Tools
export AR = ar
export NASM = nasm
export CC = gcc
export MV = mv
export RM = rm

export OPT = -O2
#export OPT = -O0 -fno-inline-functions
export WARN = -Wall -Wextra -Wno-unused-function -Wno-unused-parameter
export INCDIRS = -I$(shell pwd)/include
export CFLAGS =  $(OPT) $(WARN) -std=c99 $(INCDIRS) $(ARCH_CFLAGS) \
                 -mno-red-zone -fvisibility=default \
                 -ffreestanding -g -nostdinc -fno-builtin \
                 -fno-asynchronous-unwind-tables \
                 -fno-zero-initialized-in-bss

export BUILD_DIR = $(shell pwd)/build
export BOOTLOADER_TARGET = $(BUILD_DIR)/bootloader.bin
export KERNEL_TARGET = $(BUILD_DIR)/kernel.bin

export KERNEL_STATIC_LIB_TARGET = $(BUILD_DIR)/kernel_static.a

export INIT_TARGET = $(BUILD_DIR)/init.bin
export FINAL_TARGET = $(BUILD_DIR)/exos.img
export UNITTESTS_TARGET = $(BUILD_DIR)/unittests

export KERNEL_BUILD_DIR = $(BUILD_DIR)/kernel
export KERNEL_TEST_BUILD_DIR = $(BUILD_DIR)/test_kernel
export INIT_BUILD_DIR = $(BUILD_DIR)/init
export UNITTESTS_BUILD_DIR = $(BUILD_DIR)/tests

$(shell mkdir -p $(BUILD_DIR) > /dev/null)

all: $(FINAL_TARGET)

$(FINAL_TARGET): $(BUILD_DIR) $(BOOTLOADER_TARGET) $(KERNEL_TARGET) $(INIT_TARGET)
	dd status=noxfer conv=notrunc if=$(BOOTLOADER_TARGET) of=$@
	dd status=noxfer conv=notrunc if=$(KERNEL_TARGET) of=$@ seek=4 obs=1024 ibs=1024
	dd status=noxfer conv=notrunc if=$(INIT_TARGET) of=$@ seek=132 obs=1024 ibs=1024

$(BUILD_DIR):
	dd status=noxfer if=/dev/zero of=$(FINAL_TARGET) obs=512 ibs=512 count=2880

tests: $(UNITTESTS_TARGET)

$(UNITTESTS_TARGET):
	cd src && $(MAKE) TEST=1 BUILD_DIR=$(KERNEL_TEST_BUILD_DIR)
	cd unittests && $(MAKE) BUILD_DIR=$(UNITTESTS_BUILD_DIR)
	
clean:
	cd src && $(MAKE) clean
	cd unittests && $(MAKE) clean
	cd init_src && $(MAKE) clean
	rm -rf $(BUILD_DIR)

# Targets that do not generate files
.PHONY: clean tests $(KERNEL_TARGET) $(BOOTLOADER_TARGET) $(INIT_TARGET) $(UNITTESTS_TARGET)

$(BOOTLOADER_TARGET):
	cd bootloader && $(MAKE)

$(KERNEL_TARGET):
	cd src && $(MAKE) BUILD_DIR=$(KERNEL_BUILD_DIR)

$(INIT_TARGET):
	cd init_src && $(MAKE) BUILD_DIR=$(INIT_BUILD_DIR)

