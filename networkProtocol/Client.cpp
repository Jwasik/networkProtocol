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
	userText.setString("Type here");

	background.setFillColor(sf::Color(50, 50, 50));
	window.setFramerateLimit(60);

	this->deliveredColor = sf::Color(192, 192, 192);
	this->readedColor = sf::Color(255, 255, 255);
	this->sentColor = sf::Color(10, 10, 10);
	this->clientColor = sf::Color(50, 170, 0);
	this->errorColor = sf::Color(255, 0, 0);
	this->serverColor = sf::Color(204, 204, 0);

	addMessage("client working on " + sf::IpAddress::getLocalAddress().toString(), 0 - 1);
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
					//40 liter mieœci siê na ekranie
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
							std::cout << "Command interpreted" << std::endl;
						}
						else
						{
							addMessage(this->userText.getString().toAnsiString(), messageId);

							std::vector<UINT8> msg = toUINTtab(this->userText.getString().toAnsiString());
							Comunicate message{ 7,0,this->messageId,this->sessionId,this->userText.getString().toAnsiString().length(),msg };

							sf::Packet messagePacket;
							messagePacket << message;
							this->udpSocket.send(messagePacket, this->serverIP, this->serverPort);
							this->messageId++;
						}
						std::cout << "Clearing string" << std::endl;
						this->userText.setString("");
						this->userText.setPosition(sf::Vector2f(10, 728));
						std::cout << "String cleared" << std::endl;
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
					std::cout << "rec" << (int)receivedComunicate.operation << ' ' << (int)receivedComunicate.answer << std::endl;
					if (receivedComunicate.sessionId != this->sessionId)
					{
						std::cout << "wrong session ID " << receivedComunicate.sessionId << std::endl;
						std::cout << "should be " << this->sessionId << std::endl;
						break;
					}

					if (receivedComunicate.operation == 2)//invite
					{
						if (receivedComunicate.answer == 0)
						{
							//odebranie zaproszenia
							addMessage("\nYou have new invitation. Type /accept or /deny", 0 - 2);

							//wys³anie potwierdzenia
							ackMessage(receivedComunicate.messageId);
						}
						if (receivedComunicate.answer == 1)//accept
						{
							//odebranie odpowiedzi
							addMessage("\nClient accepted you invitation", 0 - 2);

							//wys³anie potwierdzenia
							ackMessage(receivedComunicate.messageId);
						}
						if (receivedComunicate.answer == 2)//deny
						{
							//odebranie odpowiedzi
							addMessage("\nClient refused you invitation", 0 - 2);

							//wys³anie potwierdzenia
							ackMessage(receivedComunicate.messageId);
						}
						if (receivedComunicate.answer == 3)//b³¹d
						{
							for (auto& message : messages)
							{
								if (message.first == receivedComunicate.messageId)
								{
									if (message.second.getFillColor() == deliveredColor)
									{
										message.second.setFillColor(readedColor);
									}
									else message.second.setFillColor(deliveredColor);
								}
							}
							addMessage("Cannot invite client", 0 - 2);
						}
					}

					else if (receivedComunicate.operation == 7)//msg
					{
						if (receivedComunicate.answer == 0)//msg 
						{
							//odebranie tekstu wiadomoœci
							addMessage(this->to_string(receivedComunicate), 0 - 3);

							//wys³anie potwierdzenia odebranie
							ackMessage(receivedComunicate.messageId);

						}
						if (receivedComunicate.answer == 3)//msg ack
						{
							for (auto& message : messages)
							{
								if (message.first == receivedComunicate.messageId)
								{
									if (message.second.getFillColor() == deliveredColor)
									{
										message.second.setFillColor(readedColor);
									}
									else message.second.setFillColor(deliveredColor);
								}
							}
						}
						if (receivedComunicate.answer == 7)//msg server
						{
							addMessage(this->to_string(receivedComunicate), 0 - 1);
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

void Client::addMessage(std::string message, int16_t messageId = 0)
{
	sf::Text newMessage;
	newMessage.setFont(chatFont);
	newMessage.setCharacterSize(30);
	newMessage.setPosition(sf::Vector2f(10, 693));
	newMessage.setString(message);
	if (messageId == 0)newMessage.setFillColor(serverColor);
	else if (messageId == 0 - 1)newMessage.setFillColor(errorColor);
	else if (messageId == 0 - 2)newMessage.setFillColor(serverColor);
	else if (messageId == 0 - 3)newMessage.setFillColor(clientColor);
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
		if (this->serverIP != sf::IpAddress::None)
		{
			addMessage("\nYou are already connected. Type /disconnect to end session " + this->serverIP.toString(), 0 - 1);
			return;
		}
		t1.erase(0, 6);
		this->serverIP = sf::IpAddress(t1);

		sf::Packet joinPacket;
		Comunicate c1 = { 1,0,0,0,0,std::vector<UINT8>() };
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
				addMessage("unable to reach " + this->serverIP.toString(), 0 - 1);
				this->serverIP = sf::IpAddress::None;
				return;
			}
		}
		Comunicate receivedComunicate;
		joinPacket >> receivedComunicate;

		if (receivedComunicate.operation == 1 && receivedComunicate.answer == 7)
		{
			this->sessionId = receivedComunicate.sessionId;
			addMessage("succesfully joined server " + this->serverIP.toString(), 0 - 1);
			addMessage("session id is: " + std::to_string(receivedComunicate.sessionId), 0 - 1);
		}
		else if(receivedComunicate.operation == 1 && receivedComunicate.answer == 6)
		{
			std::cout << "rec "<<receivedComunicate.operation << "---"+receivedComunicate.answer<< std::endl;
			addMessage("cannot join server, server full", 0 - 1);
			return;
		}
		else
		{
			addMessage("server cannot answer", 0 - 1);
			return;
		}

		std::cout << "Joined succesfully" << std::endl;
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/invite")))
	{
		addMessage("Invite sent", messageId);
		Comunicate c1 = { 2,0,messageId,this->sessionId,0,std::vector<UINT8>() };

		sf::Packet invitePacket;
		invitePacket << c1;

		this->udpSocket.send(invitePacket, this->serverIP, this->serverPort);
		messageId++;
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/accept.*")))
	{
		addMessage("Acceptation sent", messageId);
		Comunicate c1 = { 2,1,messageId,this->sessionId,0,std::vector<UINT8>() };

		sf::Packet invitePacket;
		invitePacket << c1;

		this->udpSocket.send(invitePacket, this->serverIP, this->serverPort);
		messageId++;
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/deny.*")))
	{
		addMessage("Rejection sent", messageId);
		Comunicate c1 = { 2,2,messageId,this->sessionId,0,std::vector<UINT8>() };

		sf::Packet invitePacket;
		invitePacket << c1;

		this->udpSocket.send(invitePacket, this->serverIP, this->serverPort);
		messageId++;
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/disconnect.*")))
	{
		if (this->serverIP == sf::IpAddress::None)
		{
			addMessage("You are not connected", messageId);
			return;
		}
		addMessage("Disconnecting...", messageId);
		Comunicate c1 = { 1,3,messageId,this->sessionId,0,std::vector<UINT8>() };
		sf::Packet disconnectPacket;
		disconnectPacket << c1;
		this->udpSocket.send(disconnectPacket,this->serverIP,this->serverPort);

		sf::Clock connectionClock;
		sf::IpAddress receivedAddress;
		unsigned short receivedPort = 0;
		while (this->udpSocket.receive(disconnectPacket, receivedAddress, receivedPort) != sf::Socket::Status::Done)
		{
			if (connectionClock.getElapsedTime().asSeconds() > 2)
			{
				addMessage("Unable to reach server " + this->serverIP.toString(), 0 - 1);
				break;
			}
		}
		Comunicate receivedComunicate;
		disconnectPacket >> receivedComunicate;

		if (receivedComunicate.operation == 1 && receivedComunicate.answer == 3)
		{
			addMessage("Succesfully disconnected " + this->serverIP.toString(), 0 - 1);
			this->serverIP = sf::IpAddress::None;
		}
		else
		{
			addMessage("\nUnable to reach server, performing emergency cutoff" + this->serverIP.toString(), 0 - 1);
			this->serverIP = sf::IpAddress::None;
		}
		this->serverIP == sf::IpAddress::None;
		messageId++;
	}
	else
	{
		addMessage("unrecognised command", 0 - 1);
	}
}

void Client::ackMessage(int16_t messageId)
{
	Comunicate c1{7,3,messageId,this->sessionId,0,std::vector<UINT8>()};
	sf::Packet ackPacket;
	ackPacket << c1;
	this->udpSocket.send(ackPacket, this->serverIP, this->serverPort);
}

sf::Packet& operator<<(sf::Packet& packet, Client::Comunicate& comunicate)
{
	packet.clear();
	std::string msg = "";
	msg += std::bitset< 3 >(comunicate.operation).to_string();
	msg += std::bitset< 3 >(comunicate.answer).to_string();
	msg += std::bitset< 32 >(comunicate.datasize).to_string();

	for (auto& letter : comunicate.data)
	{
		msg += std::bitset< 8 >(int(letter)).to_string();
	}

	msg += std::bitset< 32 >(comunicate.sessionId).to_string();
	msg += std::bitset< 32 >(comunicate.messageId).to_string();

	while (msg.length() % 32 != 0)
	{
		msg += '0';
	}

	std::cout << "msg.length()=" << msg.length() << std::endl;
	std::cout << "prepared " << msg << std::endl;

	UINT16 pom = 0;
	while (msg.length() > 0)
	{
		std::string pom2 = msg.substr(0, 16);
		pom = std::stoi(pom2, 0, 2);
		msg.erase(0, 16);
		packet << pom;
	}
	std::cout << "packet.size()=" << packet.getDataSize() << std::endl;

	return packet;
}

void operator>>(sf::Packet& packet, Client::Comunicate& comunicate)
{
	std::cout << "datasize " << packet.getDataSize() << std::endl;
	std::string msg = "";
	UINT32 pom;

	while (!packet.endOfPacket())
	{
		packet >> pom;
		std::string s = std::bitset< 32 >(pom).to_string();
		msg += s;
	}
	std::cout << "received bytes " << msg.length() << std::endl;
	std::cout << "received " << msg << std::endl;

	std::string pom2 = msg.substr(0, 3);
	msg.erase(0, 3);
	comunicate.operation = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 3);
	msg.erase(0, 3);
	comunicate.answer = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 32);
	msg.erase(0, 32);
	comunicate.datasize = std::stoi(pom2, 0, 2);

	for (int i = 0; i < comunicate.datasize; i++)
	{
		pom2 = msg.substr(0, 8);
		msg.erase(0, 8);
		UINT8 data;
		if (pom2.length() != 0)
		{
			data = std::stoi(pom2, 0, 2);
			comunicate.data.push_back(data);
		}
	}

	pom2 = msg.substr(0, 32);
	msg.erase(0, 32);
	comunicate.sessionId = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 32);
	std::cout << pom2.length() << std::endl;
	msg.erase(0, 32);
	comunicate.messageId = std::stoi(pom2, 0, 2);
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
