#include "Client.h"

Client::Client() : window(sf::VideoMode(1024, 768), "Chat Client"), background(sf::Vector2f(1024, 768))
{
	serverPort = 8888;
	sessionId = 0;
	udpSocket.bind(sf::Socket::AnyPort);
	udpSocket.setBlocking(false);
	sessionId = 0;
	serverIP = sf::IpAddress::None;

	if (!chatFont.loadFromFile("mytype.ttf"))
	{
		std::cout << "cannot load font" << std::endl;
	}
	if (!systemFont.loadFromFile("mytype.ttf"))
	{
		std::cout << "cannot load system font" << std::endl;
	}

	userText.setFont(chatFont);
	userText.setCharacterSize(30);
	userText.setFillColor(sf::Color(51, 153, 255));
	userText.setPosition(sf::Vector2f(10, 728));
	userText.setString("Text test");

	background.setFillColor(sf::Color(50, 50, 50));
	window.setFramerateLimit(60);

	for (unsigned int i = 0; i < 4; i++)
	{
		this->username += (unsigned char)rand() % 24 + 97;
	}

	addMessage("client working on " + sf::IpAddress::getLocalAddress().toString(), this->messages, this->systemFont, 0 - 1);
	addMessage("your name is " + this->username, this->messages, this->systemFont, 0 - 1);

	this->deliveredColor = sf::Color(255, 255, 255);
	this->sentColor = sf::Color(192, 192, 192);
	this->errorColor = sf::Color(255, 0, 0);
	this->serverColor = sf::Color(204, 204, 0);
}

void Client::run()
{
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::TextEntered)
			{
				if (event.text.unicode > 31 && event.text.unicode < 126) // standard letters and marks
				{
					//40 litery mieszcz¹ siê na ekranie
					if (this->userText.getString().toAnsiString().length() % 40 == 0 && this->userText.getString().toAnsiString().length() != 0)
					{
						//adds \n at the end of the line
						this->userText.setString(this->userText.getString() + "\n");
						this->userText.move(sf::Vector2f(0, -39));
						moveMessages(1, this->messages);//moves message history up
					}
					this->userText.setString(this->userText.getString() + (char)event.text.unicode);//adds new letter
				}
				else if (event.text.unicode == 8) // backspace
				{
					std::string tempString;
					tempString = this->userText.getString().toAnsiString();
					if (this->userText.getString().toAnsiString().length() % 40 == 1 && this->userText.getString().toAnsiString().length() != 1)
					{
						//deletes \n
						tempString.erase(tempString.end() - 1);
						this->userText.move(sf::Vector2f(0, 39));
						moveMessages(-1, this->messages);//moves message history down
					}
					if (tempString.length() != 0)
					{
						//deletes last char (backspace)
						tempString.erase(tempString.end() - 1);
						this->userText.setString(tempString);
					}
				}
				else if (event.text.unicode == 13) // enter
				{
					if (this->userText.getString().toAnsiString().length() != 0)
					{
						if (this->userText.getString().toAnsiString()[0] == '/')
						{
							interpreteCommand(this->userText.getString().toAnsiString(), this->messages, this->chatFont);
						}
						else
						{
							addMessage(this->userText.getString().toAnsiString(), this->messages, this->chatFont, 0);

							std::vector<UINT8> msg = toUINTtab(this->userText.getString().toAnsiString());
							Comunicate message{ 7,0,messageId,this->sessionId,this->userText.getString().toAnsiString().length(),msg };

							sf::Packet messagePacket;
							messagePacket << message;
							this->udpSocket.send(messagePacket, this->serverIP, this->serverPort);
						}
						this->userText.setString("");
						this->userText.setPosition(sf::Vector2f(10, 728));

					}

				}
			}
		}
		///----------------------------------------------------------------------------------------------------------------------------------------------------------------
		sf::Clock connectionClock;
		connectionClock.restart();

		{
			sf::Packet receivedPacket;
			Comunicate receivedComunicate;
			sf::IpAddress receivedIP;
			unsigned short receivedPort;

			while (connectionClock.getElapsedTime().asMilliseconds() < 10)
			{
				if (this->udpSocket.receive(receivedPacket, receivedIP, receivedPort) == sf::Socket::Done)
				{
					receivedPacket >> receivedComunicate;
					std::cout << "---" << receivedComunicate.operation << ' ' << receivedComunicate.answer << std::endl;
					if (receivedComunicate.sessionId != this->sessionId)
					{
						std::cout << "---" << receivedComunicate.operation << ' ' << receivedComunicate.answer << std::endl;
						std::cout << "wrong session ID " << receivedComunicate.sessionId << std::endl;
						std::cout << "should be " << this->sessionId << std::endl;
						break;
					}
					if (receivedComunicate.operation == 3)
					{

					}

					else if (receivedComunicate.operation == 7)
					{
						if (receivedComunicate.answer == 7)
						{
							addMessage(this->to_string(receivedComunicate), this->messages, this->systemFont, 0 - 1);
						}
					}
				}
			}
		}

		window.clear();
		window.draw(background);
		window.draw(this->userText);
		for (const auto& message : this->messages)
		{
			window.draw(message.second);
		}
		window.display();
	}
}

void Client::addMessage(std::string message, std::vector<std::pair<unsigned int, sf::Text>>& messages, sf::Font& chatFont, int16_t messageId = 0)
{
	sf::Text newMessage;
	newMessage.setFont(chatFont);
	newMessage.setCharacterSize(30);
	newMessage.setPosition(sf::Vector2f(10, 693));
	newMessage.setString(message);
	if (messageId == 0)newMessage.setFillColor(sentColor);
	else if (messageId == 0 - 1)newMessage.setFillColor(errorColor);
	else if (messageId == 0 - 2)newMessage.setFillColor(serverColor);
	else newMessage.setFillColor(sentColor);

	moveMessages(1, messages);
	moveMessages((message.length() / 40) * -1, messages);
	messages.push_back(std::pair<unsigned int, sf::Text>(messageId, newMessage));
	moveMessages(message.length() / 40, messages);
}

void Client::moveMessages(sf::Vector2f vector, std::vector<std::pair<unsigned int, sf::Text>>& messages)
{
	for (auto& message : messages)
	{
		message.second.move(vector);
	}
}

void Client::moveMessages(int rows, std::vector<std::pair<unsigned int, sf::Text>>& messages)
{
	unsigned int spacing = this->chatFont.getLineSpacing(30);
	for (auto& message : messages)
	{
		message.second.move(sf::Vector2f(0, -39 * rows));
	}
}

void Client::interpreteCommand(std::string t1, std::vector<std::pair<unsigned int, sf::Text>>& messages, sf::Font& chatFont)
{
	std::cmatch match;
	if (std::regex_match(t1.begin(), t1.end(), std::regex("^/join.*")))
	{
		t1.erase(0, 6);
		this->serverIP = sf::IpAddress(t1);

		sf::Packet joinPacket;
		Comunicate c1 = { 1,0,0,0,{this->username.length()},{toUINTtab(this->username)}, };
		joinPacket << c1;
		this->udpSocket.send(joinPacket, this->serverIP, this->serverPort);

		sf::Clock connectionClock;
		connectionClock.restart();

		sf::IpAddress receivedAddress;
		unsigned short receivedPort = 0;
		while (this->udpSocket.receive(joinPacket, receivedAddress, receivedPort) != sf::Socket::Status::Done)
		{
			if (connectionClock.getElapsedTime().asSeconds() > 2)
			{
				addMessage("unable to reach " + this->serverIP.toString(), messages, this->systemFont, 0 - 1);
				return;
			}
		}
		Comunicate receivedComunicate;
		joinPacket >> receivedComunicate;
		std::cout << "---" << int(receivedComunicate.operation) << '-' << int(receivedComunicate.answer) << std::endl;

		if (receivedComunicate.operation == 1 && receivedComunicate.answer == 7)
		{
			this->sessionId = receivedComunicate.sessionId;
			addMessage("succesfully joined server " + this->serverIP.toString(), messages, this->systemFont, 0 - 1);
			addMessage("session id is: " + std::to_string(receivedComunicate.sessionId), messages, this->systemFont, 0 - 1);
		}
		else
		{
			addMessage("server sent wrong answer ", messages, this->systemFont, 0 - 1);
			return;
		}
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/invite.*")))
	{

	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/create.*")))
	{

	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/leave.*")))
	{

	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/accept.*")))
	{

	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/deny.*")))
	{

	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/exit.*")))
	{

	}
	else
	{
		addMessage("unrecognised command", messages, this->systemFont, 0 - 1);
	}
}

sf::Packet& operator<<(sf::Packet& packet, Client::Comunicate& comunicate)
{
	packet << comunicate.operation << comunicate.answer << comunicate.messageId << comunicate.sessionId << comunicate.datasize;
	for (auto& letter : comunicate.data)
	{
		packet << letter;
	}
	return packet;
}

void operator>>(sf::Packet& packet, Client::Comunicate& comunicate)
{
	packet >> comunicate.operation >> comunicate.answer >> comunicate.messageId >> comunicate.sessionId >> comunicate.datasize;
	UINT8 data;
	for (uint32_t i = 0; i < comunicate.datasize; i++)
	{
		packet >> data;
		comunicate.data.push_back(data);
	}
}

std::string Client::to_string(Comunicate& com1)
{
	std::string text;
	for (auto& it : com1.data)
	{
		std::cout << it;
		text += (char)it;
	}
	std::cout << std::endl << text << std::endl;
	return text;
}

std::vector<UINT8> Client::toUINTtab(std::string string)
{
	std::vector<UINT8> vector;
	for (const auto& letter : string)
	{
		vector.push_back((UINT8)letter);
	}
	return vector;
}
