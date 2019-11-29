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
		while (window.pollEvent(event))//przechwytywanie eventów okna
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::TextEntered)
			{
				if (event.text.unicode > 31 && event.text.unicode < 126) // przechwytywanie znaków z klawiatury (litery cyfry znaki specjalne)
				{
					if (this->userText.getString().toAnsiString().length() % 40 == 0 && this->userText.getString().toAnsiString().length() != 0)//jeœli wpisany tekst jest d³u¿szy ni¿ 40 znaków
					{
						//dodanie \n na koæu linii
						this->userText.setString(this->userText.getString() + "\n");
						this->userText.move(sf::Vector2f(0, -39));
						moveMessages(1);//moves message history up
					}
					this->userText.setString(this->userText.getString() + (char)event.text.unicode);//adds new letter
				}
				else if (event.text.unicode == 8) // backspace
				{
					std::string tempString;
					tempString = this->userText.getString().toAnsiString();
					if (this->userText.getString().toAnsiString().length() % 40 == 1 && this->userText.getString().toAnsiString().length() != 1)
					{//jeœli usuwamy pierwszy znak z linii
						//deletes \n
						tempString.erase(tempString.end() - 1);
						this->userText.move(sf::Vector2f(0, 39));
						moveMessages(-1);//moves message history down
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
						{//jeœli tekst zaczyna siê od / to traktujemy go jako komendê
							interpreteCommand(this->userText.getString().toAnsiString());
						}
						else
						{
							addMessage(this->userText.getString().toAnsiString(), messageId);//dodanie przechwyconego napisu do historii wiadomoœci

							std::vector<UINT8> msg = toUINTtab(this->userText.getString().toAnsiString());//przepisanie napisu na tablicê <UINT8>
							Comunicate message{ 7,0,this->messageId,this->sessionId,this->userText.getString().toAnsiString().length(),msg };//utworzenie komunikatu z wiadomoœci¹ tekstow¹
							this->send(message);//wys³anie komunikatu
						}
						this->userText.setString("");
						this->userText.setPosition(sf::Vector2f(10, 728));
						//wyzerowanie okna wejœciowego i przesuniêcie wszystkiego do pozycji pocz¹tkowej

					}
				}
			}
		}
		///----------------------------------------------------------------------------------------------------------------------------------------------------------------
		///Odbieranie pakietów
		sf::Clock connectionClock;
		connectionClock.restart();

		{
			//Poni¿sze zmienne s³u¿¹ do przechowywania odebranych informacji
			sf::Packet receivedPacket;//odebrany pakiet
			Comunicate receivedComunicate;//komunikat do którego zostanie przepisany pakiet
			sf::IpAddress receivedIP;//IP nadawcy
			unsigned short receivedPort;//port nadawcy

			while (connectionClock.getElapsedTime().asMilliseconds() < 10)//odbieramy wiadomoœci maksymalnie przez 10ms na jedn¹ klatkê programu
			{
				if (this->udpSocket.receive(receivedPacket, receivedIP, receivedPort) == sf::Socket::Done)//jeœli uda³o siê odebraæ pakiet
				{
					receivedPacket >> receivedComunicate;//przepisujemy pakiet do komunikatu
					if (receivedComunicate.sessionId != this->sessionId)
					{//gdy IdSesji jest b³êdne, koñczymy dzia³anie programu. Mo¿liwa próba w³amania
						std::cout << "wrong session ID " << receivedComunicate.sessionId << std::endl;
						std::cout << "should be " << this->sessionId << std::endl;
						break;
					}

					if (receivedComunicate.operation == 7 && receivedComunicate.answer == 3)//msg ack
					{//odebranie potwierdzenia wiadomoœci
						for (auto& message : messages)
						{
							if (message.first == receivedComunicate.messageId)
							{
								if (message.second.getFillColor() == deliveredColor)
								{
									message.second.setFillColor(readedColor);//kolorujemy wiadomoœæ w historii jako odczytan¹
								}
								else message.second.setFillColor(deliveredColor);//kolorujemy wiadomoœæ w historii jako dostarczon¹
							}
						}
					}
					else
					{
						this->ackMessage(receivedComunicate.messageId);//potwierdŸ odebranie wiadomoœci zaraz po otrzymaniu pakietu

						///Interpretacja odebraniej wiadomoœci

						//operation == 2 to komunikaty dot. zaproszeñ
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
							if (receivedComunicate.answer == 3)//odmowa zaproszenia 
							{
								addMessage("Cannot invite client", receivedComunicate.messageId);
							}
						}///Operation == 3 to komunikaty diagnostyczne (wiadomoœci od serwera dla u¿ytkownika)
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
						else if (receivedComunicate.operation == 7 && receivedComunicate.answer == 0)//pakiet z wiadomoœci¹ tekstow¹
						{
							addMessage(this->to_string(receivedComunicate), 0 - 3);
							//zapisuje wiadomoœæ w historii
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
{//procedura dodaj¹ca wiadomoœæ do historii oraz przesuwaj¹ca ca³¹ historiê ca³y czas do góry
	sf::Text newMessage;
	newMessage.setFont(chatFont);
	newMessage.setCharacterSize(30);
	newMessage.setPosition(sf::Vector2f(10, 693));
	newMessage.setString(message);
	//kolorowanie wiadomoœci w historii
	if (messageId >= 16384 || messageId == 0)newMessage.setFillColor(serverColor);//jeœli id wiadomoœci nale¿y do serwera to kolorujemy wiadomoœæ na ¿ó³to
	else if (messageId == 0 - 1)newMessage.setFillColor(errorColor);//wiadomoœæ bêd¹ca b³êdem lub informacj¹ od aplikacji
	else if (messageId == 0 - 3)newMessage.setFillColor(clientColor);//wiadomoœæ od innego klienta
	else newMessage.setFillColor(sentColor);//w przeciwnym wypadku wiadomoœæ ma kolor czarny, który zmieni siê jeœli zostanie potwierdzona

	moveMessages(1);//przesuwa historiê w górê
	moveMessages((message.length() / 40) * -1);
	messages.push_back(std::pair<unsigned int, sf::Text>(messageId, newMessage));
	moveMessages(message.length() / 40);
}

void Client::moveMessages(sf::Vector2f vector)
{//przesuwanie historii wiadomoœci o wektor
	for (auto& message : messages)
	{
		message.second.move(vector);
	}
}

void Client::moveMessages(int rows)
{//przesuwanie historii wiadomoœci o liczbê wierszy
	unsigned int spacing = this->chatFont.getLineSpacing(30);
	for (auto& message : messages)
	{
		message.second.move(sf::Vector2f(0, -39 * rows));
	}
}

void Client::interpreteCommand(std::string t1)
{//procedura interpretuj¹ca komendy
	std::cmatch match;
	if (std::regex_match(t1.begin(), t1.end(), std::regex("^/join.*")))//komenda /join (do³¹cz do serwera)
	{
		if (this->serverIP != sf::IpAddress::None)//jeœli ju¿ jesteœmy po³¹czeni
		{
			addMessage("\nYou are already connected. Type /disconnect to end session " + this->serverIP.toString(), 0 - 1);
			return;
		}
		t1.erase(0, 6);//usuwamy "/join " z komendy tak aby pozosta³o samo IP
		this->serverIP = sf::IpAddress(t1);//ustawiamy adres IP serwera, port jest sta³y i wynosi 8888

		sf::Packet joinPacket;
		Comunicate c1 = { 1,0,0,0,0,std::vector<UINT8>() };//tworzymy komunikat join
		this->send(c1);//wysy³amy komunikat

		sf::Clock connectionClock;
		connectionClock.restart();

		sf::IpAddress receivedAddress;
		unsigned short receivedPort = 0;
		while (this->udpSocket.receive(joinPacket, receivedAddress, receivedPort) != sf::Socket::Status::Done)
		{//czekamy na odpowiedŸ od serwera
			if (connectionClock.getElapsedTime().asSeconds() > 2)
			{//jeœli od wys³ania komunikatu minê³y 2 sekundy
				addMessage("unable to reach " + this->serverIP.toString(), 0 - 1);
				this->serverIP = sf::IpAddress::None;
				return;
			}
		}
		Comunicate receivedComunicate;
		joinPacket >> receivedComunicate;

		//interpretujemy odpowiedŸ serwera
		if (receivedComunicate.operation == 1 && receivedComunicate.answer == 7)
		{//jeœli siê zgodzi³ na rozpoczêcie sesji
			this->sessionId = receivedComunicate.sessionId;//ustawiamy id sesji na to podane przez serwer
			addMessage("succesfully joined server " + this->serverIP.toString(), receivedComunicate.messageId);
			addMessage("session id is: " + std::to_string(receivedComunicate.sessionId), receivedComunicate.messageId);

			c1 = Comunicate{ 1,5,receivedComunicate.messageId,this->sessionId,0,std::vector<UINT8>() };//wysy³amy potwierdzenie do³¹czenia do serwera
			this->retransmit(c1);
		}
		else if (receivedComunicate.operation == 1 && receivedComunicate.answer == 6)
		{//jeœli serwer jest pe³ny
			std::cout << "rec " << receivedComunicate.operation << "---" + receivedComunicate.answer << std::endl;
			addMessage("cannot join server, server full", receivedComunicate.messageId);
			this->ackMessage(receivedComunicate.messageId);//potwierdzamy otrzymanie wiadomoœci
			return;
		}
		else
		{//gdy serwer wyœle b³êdn¹ odpowiedŸ
			addMessage("server cannot answer", 0 - 1);
			return;
		}
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/invite")))
	{//wys³anie zaproszenia
		addMessage("Invite sent", messageId);
		Comunicate c1 = { 2,0,messageId,this->sessionId,0,std::vector<UINT8>() };

		this->send(c1);
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/accept.*")))
	{//zaakceptowanie zaproszenia
		addMessage("Acceptation sent", messageId);
		Comunicate c1 = { 2,1,messageId,this->sessionId,0,std::vector<UINT8>() };

		this->send(c1);
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/deny.*")))
	{//odmowa zaproszenia
		addMessage("Rejection sent", messageId);
		Comunicate c1 = { 2,2,messageId,this->sessionId,0,std::vector<UINT8>() };

		this->send(c1);
	}
	else if (std::regex_match(t1.begin(), t1.end(), std::regex("^/disconnect.*")))
	{//od³¹czenie siê od serwera
		if (this->serverIP == sf::IpAddress::None)
		{//gdy nie jesteœmy pod³¹czeni
			addMessage("You are not connected", 0-1);
			return;
		}
		addMessage("Disconnecting...", messageId);
		Comunicate c1 = { 1,3,messageId,this->sessionId,0,std::vector<UINT8>() };//wys³anie komunikatu koñcz¹cego sesjê
		this->send(c1);

		sf::Packet receivedPacket;
		sf::Clock connectionClock;
		sf::IpAddress receivedAddress;
		unsigned short receivedPort = 0;
		while (this->udpSocket.receive(receivedPacket, receivedAddress, receivedPort) != sf::Socket::Status::Done)
		{//oczekiwanie na odpowiedŸ serwera
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
			c1 = Comunicate{ 1,1,messageId,this->sessionId,0,std::vector<UINT8>() };//wys³anie potwierdzenia od³¹czenia od serwera
			this->send(c1);
			this->serverIP = sf::IpAddress::None;
		}
		else
		{//gdy serwer nie odpowie roz³¹czamy siê awaryjnie (po prostu zamykamy po³¹czenie)
			addMessage("\nUnable to reach server, performing emergency cutoff" + this->serverIP.toString(), 0 - 1);
			this->serverIP = sf::IpAddress::None;
		}
		this->serverIP == sf::IpAddress::None;
	}
	else
	{//gdy nie rozpoznano wiadomoœci
		addMessage("unrecognised command", 0 - 1);
	}
}

void Client::ackMessage(int16_t messageId)
{//procedura wysy³aj¹ca potwierdzenie do wiadomoœci o zadanym Id
	Comunicate c1{ 7,3,messageId,this->sessionId,0,std::vector<UINT8>() };
	sf::Packet ackPacket;
	ackPacket << c1;
	this->udpSocket.send(ackPacket, this->serverIP, this->serverPort);
}

void Client::send(Comunicate& com)
{//procedura wysy³aj¹ca wiadomoœæ i nadaj¹ce jej Id
	com.messageId = this->messageId;
	this->messageId++;
	sf::Packet packet;
	packet << com;
	this->udpSocket.send(packet, this->serverIP, this->serverPort);

}

void Client::retransmit(Comunicate& com)
{//procedura wysy³aj¹ca wiadomoœæ bez zmiany jej Id
	sf::Packet packet;
	packet << com;
	this->udpSocket.send(packet, this->serverIP, this->serverPort);
}

sf::Packet& operator<<(sf::Packet& packet, Client::Comunicate& comunicate)
{//przesy³aniekomunikatu do pakietu
	packet.clear();
	std::string msg = "";//najpierw tworzymy string
	msg += std::bitset< 3 >(comunicate.operation).to_string();//dodajemy do stringa kolejne pola zamieniaj¹c je na liczby binarne w postaci ci¹gu znaków
	msg += std::bitset< 3 >(comunicate.answer).to_string();//argument <3> oznacza ¿e zamieniamy comunicate.answer na 3-bitow¹ liczbê zapisan¹ jako ci¹g znaków
	msg += std::bitset< 32 >(comunicate.datasize).to_string();//dodajemy pole rozmiaru danych

	for (auto& letter : comunicate.data)//dodajemy wszystkie litery z komunikatu
	{
		msg += std::bitset< 8 >(int(letter)).to_string();
	}

	msg += std::bitset< 32 >(comunicate.sessionId).to_string();//dodajemy id sesji
	msg += std::bitset< 32 >(comunicate.messageId).to_string();//dodajemy id wiadomoœci

	while (msg.length() % 8 != 0)//uzupe³niamy zerami do d³ugoœci wielokrotnoœci liczby 8
	{
		msg += '0';
	}
	
	UINT8 pom = 0;
	while (msg.length() > 0)
	{
		std::string pom2 = msg.substr(0, 8);//wczytujemy 8 bitów ze stringa do zmiennej pomocniczej
		pom = std::stoi(pom2, 0, 2);//teraz zamieniamy 8 bitów ze zmiennej pomocniczej na liczbê reprezentowan¹ przez te 8 bitów
		msg.erase(0, 8);//wycinamy ze stringa 8 bitów
		packet << pom;//wrzucamy 8 bitów do pakietu. Czynnoœæ powtarzamy a¿ string bêdzie pusty
	}
	return packet;
}

void operator>>(sf::Packet& packet, Client::Comunicate& comunicate)
{
	std::string msg = "";
	UINT8 pom;

	while (!packet.endOfPacket())
	{
		packet >> pom;//wczytujemy 8 bitów pakietu do zmiennej 8-bitowej
		std::string s = std::bitset< 8 >(pom).to_string();//zamieniamy zmienn¹ 8 bitow¹ na ci¹g znaków binarnych i dodajemy do stringa
		msg += s;
	}

	std::string pom2 = msg.substr(0, 3);//wyci¹gamy 3 bity ze stringa
	msg.erase(0, 3);
	if (pom2.length() == 0)return;
	comunicate.operation = std::stoi(pom2, 0, 2);//zamieniamy wyci¹gniête 3 bity na zmienn¹ i zapisujemy je do komunikatu
	//czynoœæ powtarzamy dla wszystkich pól

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
{//zamiana zawartoœci komunikatu na ³atwy do wyœwietlenia tekst
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
{//zamiania tablicy string, na wektor <UINT8>
	std::vector<UINT8> vector;
	for (const auto& letter : string)
	{
		vector.push_back((UINT8)letter);
	}
	return vector;
}
