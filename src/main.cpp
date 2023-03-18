#include <cstdio>
#include <csignal>
#include <memory>

#include "dmg.h"

std::weak_ptr<Dmg> dmg_ptr;

void sigint(int)
{
	if (!dmg_ptr.expired())
		dmg_ptr.lock()->power_off();
}

int main(int argc, char* argv[])
{
	signal(SIGINT, sigint);

	auto dmg = std::allocate_shared<Dmg>(std::allocator<Dmg>());
	dmg_ptr = dmg;

	if (argc == 2)
		dmg->insert_cartridge(argv[1]);
	else
		dmg->insert_cartridge("roms/test-loop.gb");

	dmg->power_on();

	printf("Goodbye!\n");

	return 0;
}
