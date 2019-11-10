#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <regex>
#include <String>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include "Client.h"

int main()
{
	Client client;
	srand(time(NULL));

	client.run();
	return 0;
}


