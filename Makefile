ODIR=build

CXX=g++
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
-nostdlib \
-Wall \
-Wextra \
-no-pie \
-g \
-O0 -pipe -Wno-unused-parameter -Wno-register \
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

kernel: $(OBJS)
	$(CXX) -T scripts/link.ld -o kernel.elf $(CXXFLAGS) $(OBJS)
	mv kernel.elf hdd_root

clean:
	rm -rf $(OBJS)

run: kernel
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,file=OVMF_CODE.fd -drive if=pflash,format=raw,file=OVMF_VARS.fd -serial stdio -d int -no-reboot -no-shutdown \
		-drive file=fat:rw:hdd_root,format=raw,media=disk -m 2G -cpu qemu64,+avx,+avx2 -smp 4 -M q35 -vga vmware