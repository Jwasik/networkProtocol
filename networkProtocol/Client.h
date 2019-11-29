#pragma once
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <regex>
#include <String>
#include <bitset>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>


class Client
{
private:
	//kolory wiadomo�ci
	sf::Color deliveredColor;
	sf::Color readedColor;
	sf::Color sentColor;
	sf::Color errorColor;
	sf::Color serverColor;
	sf::Color clientColor;

	struct Comunicate//struktura komunikatu
	{
		UINT8 operation;//3b
		UINT8 answer;//3b
		uint16_t messageId;//16b
		uint32_t sessionId;//32b
		uint32_t datasize;//32b
		std::vector<UINT8> data;
	};

	uint16_t messageId = 1;//pocz�tkowa warto�� numeru wiadomo�ci
	sf::RenderWindow window;//okno programu
	sf::RectangleShape background;//t�o okna
	bool endSession = 0;//flaga zamykaj�ca sesj�
	sf::Text userText;//pole do kt�rego u�ytkownik wprowadza tekst
	sf::Font chatFont;//czcionka chatu
	std::vector<std::pair<unsigned int, sf::Text>> messages;//historia wiadomo�ci
	sf::UdpSocket udpSocket;//g��wny socket komunikacyjny
	sf::IpAddress serverIP;//zmienna przechowuj�ca ip serwera
	unsigned short serverPort;//zmienna przechowuj�ca port serwera
	unsigned int sessionId;//id sesji

	friend sf::Packet& operator<<(sf::Packet&, Comunicate&);
	friend void operator>>(sf::Packet&, Comunicate&);
	std::string to_string(Comunicate&);//metoda zamieniaj�ca komunikat na �atwy do wypisania tekst

	std::vector<UINT8> toUINTtab(std::string string);//metoda zamieniaj�ca tablic� string, na wektor <UINT8>
	void addMessage(std::string,int16_t);//metoda dodaj�ca wiadomo�� do historii, przyjmuje tekst wiadomo�ci oraz id woadomo�ci
	void moveMessages(sf::Vector2f);//metoda przesuwaj�ca histri� wiadomo�ci o zadany wektor
	void moveMessages(int);//metoda przesuwaj�ca histri� wiadomo�ci o zadan�ilo��wierszy
	void interpreteCommand(std::string);//procedura interpretuj�ca polecenia u�ytkownika
	void ackMessage(int16_t);//procedura wysy�aj�ca potwierdzenie wiadomo�ci o podanym id
	void send(Comunicate&);//procedura wysy�aj�ca komunikat do serwera
	void retransmit(Comunicate&);//procedura retransmituj�ca komunikat do serwera bez zmiany id wiadomo�ci

public:
	Client();
	void run();
};
