#include "player.h"
#include <cstdio>
#include <thread>
#include <chrono>
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: LossCat <file> \n");
		return -1;
	}
	Player player;
	if (!player.init(0,0))
	{
		return 1;
	}
	if (!player.load(argv[1]))
	{
		return 1;
	}
	player.play();
	while (!player.isFinished())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	return 0;

}