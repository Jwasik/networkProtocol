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
							std::cout << "sending message no: " << this->messageId << std::endl;
							this->send(message);
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
					if (receivedComunicate.sessionId != this->sessionId)
					{
						std::cout << "wrong session ID " << receivedComunicate.sessionId << std::endl;
						std::cout << "should be " << this->sessionId << std::endl;
						break;
					}

					if (receivedComunicate.operation == 7 && receivedComunicate.answer == 3)//msg ack
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
					else
					{
						this->ackMessage(receivedComunicate.messageId);//potwierdŸ odebranie wiadomoœci

						if (receivedComunicate.operation == 2)//invite
						{
							if (receivedComunicate.answer == 0)
							{
								//odebranie zaproszenia
								addMessage("\nYou have new invitation. Type /accept or /deny", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 1)//accept
							{
								//odebranie odpowiedzi
								addMessage("\nClient accepted you invitation", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 2)//deny
							{
								//odebranie odpowiedzi
								addMessage("\nClient refused you invitation", receivedComunicate.messageId);
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
								addMessage("Cannot invite client", receivedComunicate.messageId);
							}
						}
						else if (receivedComunicate.operation == 3)
						{
							if (receivedComunicate.answer == 0)
							{
								addMessage("You are alone on serwer", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 1)
							{
								addMessage("There is one another client", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 2)
							{
								addMessage("Not found other client", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 3)
							{
								addMessage("Client left serwer", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 4)
							{
								addMessage("Already invited", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 5)
							{
								addMessage("You invitation has been accepted", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 6)
							{
								addMessage("You invitation has been denied", receivedComunicate.messageId);
							}
							if (receivedComunicate.answer == 7)
							{
								addMessage("You are not invited", receivedComunicate.messageId);
							}
						}
						else if (receivedComunicate.operation == 7)//msg
						{
							if (receivedComunicate.answer == 0)//msg 
							{
								//odebranie tekstu wiadomoœci
								addMessage(this->to_string(receivedComunicate), 0 - 3);
							}
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
	if (messageId >= 16384 || messageId == 0)newMessage.setFillColor(serverColor);
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
		this->send(c1);

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
			addMessage("succesfully joined server " + this->serverIP.toString(), receivedComunicate.messageId);
			addMessage("session id is: " + std::to_string(receivedComunicate.sessionId), receivedComunicate.messageId);

			c1 = Comunicate{ 1,5,receivedComunicate.messageId,this->sessionId,0,std::vector<UINT8>() };
			this->retransmit(c1);
		}
		else if (receivedComunicate.operation == 1 && receivedComunicate.answer == 6)
		{
			std::cout << "rec " << receivedComunicate.operation << "---" + receivedComunicate.answer << std::endl;
			addMessage("cannot join server, server full", receivedComunicate.messageId);
			this->ackMessage(receivedComunicate.messageId);
			return;
		}
		else
		{
			addMessage("server cannot answer", 0 - 1);
			return;
		}
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/invite")))
	{
		addMessage("Invite sent", messageId);
		Comunicate c1 = { 2,0,messageId,this->sessionId,0,std::vector<UINT8>() };

		this->send(c1);
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/accept.*")))
	{
		addMessage("Acceptation sent", messageId);
		Comunicate c1 = { 2,1,messageId,this->sessionId,0,std::vector<UINT8>() };

		this->send(c1);
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/deny.*")))
	{
		addMessage("Rejection sent", messageId);
		Comunicate c1 = { 2,2,messageId,this->sessionId,0,std::vector<UINT8>() };

		this->send(c1);
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/disconnect.*")))
	{
		if (this->serverIP == sf::IpAddress::None)
		{
			addMessage("You are not connected", 0-1);
			return;
		}
		addMessage("Disconnecting...", messageId);
		Comunicate c1 = { 1,3,messageId,this->sessionId,0,std::vector<UINT8>() };
		this->send(c1);

		sf::Packet receivedPacket;
		sf::Clock connectionClock;
		sf::IpAddress receivedAddress;
		unsigned short receivedPort = 0;
		while (this->udpSocket.receive(receivedPacket, receivedAddress, receivedPort) != sf::Socket::Status::Done)
		{
			if (connectionClock.getElapsedTime().asSeconds() > 2)
			{
				addMessage("Unable to reach server " + this->serverIP.toString(), 0 - 1);
				break;
			}
		}
		Comunicate receivedComunicate;
		receivedPacket >> receivedComunicate;

		if (receivedComunicate.operation == 1 && receivedComunicate.answer == 4)
		{
			addMessage("Succesfully disconnected " + this->serverIP.toString(), 0 - 1);
			this->serverIP = sf::IpAddress::None;
			c1 = Comunicate{ 1,1,messageId,this->sessionId,0,std::vector<UINT8>() };
		}
		else
		{
			addMessage("\nUnable to reach server, performing emergency cutoff" + this->serverIP.toString(), 0 - 1);
			this->serverIP = sf::IpAddress::None;
		}
		this->serverIP == sf::IpAddress::None;
	}
	else
	{
		addMessage("unrecognised command", 0 - 1);
	}
}

void Client::ackMessage(int16_t messageId)
{
	std::cout << "sending ack to: " << messageId << std::endl;
	Comunicate c1{ 7,3,messageId,this->sessionId,0,std::vector<UINT8>() };
	sf::Packet ackPacket;
	ackPacket << c1;
	this->udpSocket.send(ackPacket, this->serverIP, this->serverPort);
}

void Client::send(Comunicate& com)
{
	com.messageId = this->messageId;
	this->messageId++;
	sf::Packet packet;
	packet << com;
	this->udpSocket.send(packet, this->serverIP, this->serverPort);

}

void Client::retransmit(Comunicate& com)
{
	sf::Packet packet;
	packet << com;
	this->udpSocket.send(packet, this->serverIP, this->serverPort);
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

	while (msg.length() % 8 != 0)
	{
		msg += '0';
	}
	std::cout << "prepared " << msg << std::endl;

	UINT8 pom = 0;
	while (msg.length() > 0)
	{
		std::string pom2 = msg.substr(0, 8);
		pom = std::stoi(pom2, 0, 2);
		msg.erase(0, 8);
		packet << pom;
	}
	return packet;
}

void operator>>(sf::Packet& packet, Client::Comunicate& comunicate)
{
	std::string msg = "";
	UINT8 pom;

	while (!packet.endOfPacket())
	{
		packet >> pom;
		std::string s = std::bitset< 8 >(pom).to_string();
		msg += s;
	}
	std::cout << "received " << msg << std::endl;

	std::string pom2 = msg.substr(0, 3);
	msg.erase(0, 3);
	if (pom2.length() == 0)return;
	comunicate.operation = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 3);
	msg.erase(0, 3);
	if (pom2.length() == 0)return;
	comunicate.answer = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 32);
	msg.erase(0, 32);
	if (pom2.length() == 0)return;
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
	if (pom2.length() != 0)comunicate.messageId = std::stoi(pom2, 0, 2);

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
