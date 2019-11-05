#pragma once
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <regex>
#include <String>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>


class Client
{
private:
	sf::Color deliveredColor;
	sf::Color sentColor;
	sf::Color errorColor;
	sf::Color serverColor;

	struct Comunicate
	{
		UINT8 operation;//3b
		UINT8 answer;//3b
		int16_t messageId;//16b
		uint32_t sessionId;//32b
		uint32_t datasize;//32b
		std::vector<UINT8> data;
	};

	uint16_t messageId = 1;
	sf::RenderWindow window;
	sf::RectangleShape background;
	bool endSession = 0;
	sf::Text userText;
	sf::Font chatFont;
	sf::Font systemFont;
	std::vector<std::pair<unsigned int, sf::Text>> messages;
	sf::UdpSocket udpSocket;
	sf::IpAddress serverIP;
	unsigned short serverPort;
	unsigned int sessionId;
	std::string username;

	friend sf::Packet& operator<<(sf::Packet&, Comunicate&);
	friend void operator>>(sf::Packet&, Comunicate&);
	std::string to_string(Comunicate&);

	std::vector<UINT8> toUINTtab(std::string string);
	void addMessage(std::string, std::vector<std::pair<unsigned int, sf::Text>>&, sf::Font&, int16_t);
	void moveMessages(sf::Vector2f, std::vector<std::pair<unsigned int, sf::Text>>&);
	void moveMessages(int, std::vector<std::pair<unsigned int, sf::Text>>&);
	void interpreteCommand(std::string, std::vector<std::pair<unsigned int, sf::Text>>&, sf::Font&);


public:
	Client();
	void run();
};
