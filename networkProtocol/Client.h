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
	//kolory wiadomoœci
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

	uint16_t messageId = 1;//pocz¹tkowa wartoœæ numeru wiadomoœci
	sf::RenderWindow window;//okno programu
	sf::RectangleShape background;//t³o okna
	bool endSession = 0;//flaga zamykaj¹ca sesjê
	sf::Text userText;//pole do którego u¿ytkownik wprowadza tekst
	sf::Font chatFont;//czcionka chatu
	std::vector<std::pair<unsigned int, sf::Text>> messages;//historia wiadomoœci
	sf::UdpSocket udpSocket;//g³ówny socket komunikacyjny
	sf::IpAddress serverIP;//zmienna przechowuj¹ca ip serwera
	unsigned short serverPort;//zmienna przechowuj¹ca port serwera
	unsigned int sessionId;//id sesji

	friend sf::Packet& operator<<(sf::Packet&, Comunicate&);
	friend void operator>>(sf::Packet&, Comunicate&);
	std::string to_string(Comunicate&);//metoda zamieniaj¹ca komunikat na ³atwy do wypisania tekst

	std::vector<UINT8> toUINTtab(std::string string);//metoda zamieniaj¹ca tablicê string, na wektor <UINT8>
	void addMessage(std::string,int16_t);//metoda dodaj¹ca wiadomoœæ do historii, przyjmuje tekst wiadomoœci oraz id woadomoœci
	void moveMessages(sf::Vector2f);//metoda przesuwaj¹ca histriê wiadomoœci o zadany wektor
	void moveMessages(int);//metoda przesuwaj¹ca histriê wiadomoœci o zadan¹iloœæwierszy
	void interpreteCommand(std::string);//procedura interpretuj¹ca polecenia u¿ytkownika
	void ackMessage(int16_t);//procedura wysy³aj¹ca potwierdzenie wiadomoœci o podanym id
	void send(Comunicate&);//procedura wysy³aj¹ca komunikat do serwera
	void retransmit(Comunicate&);//procedura retransmituj¹ca komunikat do serwera bez zmiany id wiadomoœci

public:
	Client();
	void run();
};
