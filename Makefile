ODIR=build

CXX=x86_64-linux-gnu-g++
CXXFLAGS=-std=gnu++20 \
-ffreestanding \
-fno-builtin \
-fno-stack-protector \
-fno-stack-check \
-fno-omit-frame-pointer \
-fno-lto \
-fno-PIE \
-fno-PIC \
-m64 \
-march=x86-64 \
-mabi=sysv \
-mno-red-zone \
-mcmodel=kernel \
-fno-rtti \
-fno-exceptions \
-Isrc \
-I. \
-I.. \
-I../include \
-I./src/include \
-nostdlib \
-Wall \
-Wextra \
-no-pie \
-g \
-O2 -pipe -Wno-unused-parameter -Wno-register \
-mno-80387 \
-mno-sse \
-mno-sse2 \
-mno-avx \
-mno-avx2

SOURCES := $(shell find src -type f -name *.cpp)
SOURCES_ASM := $(shell find src -type f -name *.asm)
OBJS = $(patsubst src/%,$(ODIR)/%,$(SOURCES:.cpp=.o))
OBJS += $(patsubst src/%,$(ODIR)/%,$(SOURCES_ASM:.asm=.o))

$(ODIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(ODIR)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	nasm -felf64 $< -o $@

all: disk

kernel: $(OBJS)
	$(CXX) -T scripts/link.ld -o kernel.elf $(CXXFLAGS) $(OBJS)
	mv kernel.elf hdd_root

disk: kernel
	dd of=disk.img if=/dev/zero bs=1024 count=91669
	parted disk.img -s -a minimal mklabel gpt
	parted disk.img -s -a minimal mkpart EFI FAT16 2048s 93716s
	parted disk.img -s -a minimal toggle 1 boot
	dd of=part.img if=/dev/zero bs=512 count=91669
	mformat -i part.img -h 32 -t 32 -n 64 -c 1
	mmd -i part.img ::/EFI
	mmd -i part.img ::/EFI/BOOT
	mcopy -i part.img hdd_root/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i part.img hdd_root/initrd.img ::
	mcopy -i part.img hdd_root/kernel.elf ::
	mcopy -i part.img hdd_root/limine.cfg ::
	dd if=part.img of=disk.img bs=512 count=91669 seek=2048 conv=notrunc
	rm -r part.img

clean:
	rm -rf $(OBJS)

run: disk
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,file=OVMF_CODE.fd -drive if=pflash,format=raw,file=OVMF_VARS.fd -serial stdio -no-reboot -no-shutdown \
		-drive file=disk.img,format=raw,media=disk,if=none,id=nvm -device nvme,serial=deadbeef,drive=nvm -m 2G -cpu qemu64,+avx,+avx2 -smp 4 -M q35 -vga vmware
run-debug: kernel
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,file=OVMF_CODE.fd -drive if=pflash,format=raw,file=OVMF_VARS.fd -serial stdio -no-reboot -no-shutdown \
		-drive file=fat:rw:hdd_root,format=raw,media=disk -m 2G -cpu qemu64,+avx,+avx2 -smp 4 -M q35 -vga vmware -d int -s -S &
