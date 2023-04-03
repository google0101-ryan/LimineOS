#include <stdint.h>
#include <stddef.h>
#include <kernel/limine.h>

#ifdef ARCH_AMD64
#include <kernel/arch/x86/arch.h>
#endif

#include <drivers/screen.h>

#include <kernel/vfs/vfs.h>
#include <kernel/vfs/initrd.h>
#include <kernel/vfs/rpc.h>

static volatile limine_framebuffer_request fb_req =
{
	.id = LIMINE_FRAMEBUFFER_REQUEST,
	.response = nullptr
};

void KThread()
{
	Terminal::SetScreenColors(0xFFFFFFFF, 0x00000000);

	Terminal::ClearScreen();

	printf("Entered kernel thread\n");

	VFS::Init();

	printf("Initialized VFS\n");

	Initrd* initrd = new Initrd();

	vfs->mount(initrd, "/", true);

	RpcFilesystem* rpc = new RpcFilesystem();

	vfs->mount(rpc, "/rpc", false);

	int res = vfs->LoadAndExec("/hello_world.elf");

	if (res < 0)
	{
		printf("Unhandled result %d\n", res);
		Arch::Halt();
	}

	Arch::Halt();
}

extern "C" void _start()
{
	if (fb_req.response->framebuffer_count < 1)
		Arch::Halt();

	Terminal::Init(fb_req.response);

	// This will take us into multitasking
	// We resume execution after this function in KThread()
	Arch::Init();

	Arch::Halt();
}