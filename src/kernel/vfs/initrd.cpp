#include "initrd.h"
#include <kernel/limine.h>
#include <kernel/libk/string.h>
#include <kernel/drivers/screen.h>

static volatile limine_module_request mod_req =
{
	.id = LIMINE_MODULE_REQUEST,
	.response = nullptr
};

Initrd::Initrd()
{
	printf("Initializing initrd\n");
	limine_internal_module* initrd;

	printf("Searching through %ld modules\n", mod_req.response->module_count);

	for (size_t i = 0; i < mod_req.response->module_count; i++)
	{
		auto mod = mod_req.response->modules[i];

		printf("%s %s\n", mod->path, mod->cmdline);
	}
}