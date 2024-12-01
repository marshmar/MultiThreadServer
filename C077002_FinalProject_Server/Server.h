#pragma once

#pragma comment(lib,"ws2_32")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<WinSock2.h>
#include<stdlib.h>
#include<iostream>
#include<tchar.h>
#include"ConnectionPacket.h"
#include"MovePacket.h"
#include"ChatPacket.h"


#define PORT 9000

class Server
{
private:
	//For Socket
	USHORT SERVERPORT;
	WSADATA Wsadata;		//Initiate WinSock
	SOCKET Listen_Socket;
	SOCKADDR_IN ServerAddress;

	//Variable for Data Communication

	SOCKET clientSocket;
	SOCKADDR_IN clientAddress;
	INT AddressLen;
	// ���� Ŭ���̾�Ʈ ���� �迭
	std::vector<SOCKET> clientSockets;
	// ����ȭ ó�� ���ؽ�
	HANDLE hMutex;

	VOID Error_Quit(const TCHAR* Msg);
	VOID Error_Display(const TCHAR* Msg);
	bool Accept();
	std::string GetCurrentTimeToString() const;
	void PrintClientCount() const;

	// ��Ŷ ���� �Լ���
	void SetConnectionPacket(std::vector<char>& buffer, ConnectionPacket& ConPacket, HEADER header, std::string str);
	ConnectionPacket GetConnectionPacket(std::vector<char>& buffer, HEADER header, short packetLength);
	void SetMovePacket(std::vector<char>& buffer, MovePacket& movePacket, HEADER header, short x, short y, short z, std::string str);
	MovePacket GetMovePacket(std::vector<char>& buffer, HEADER header, short packetLength);
	void SetChatPacket(std::vector<char>& buffer, ChatPacket& ChatPacket, HEADER header, std::string str);
	ChatPacket GetChatPacket(std::vector<char>& buffer, HEADER header, short packetLength);
public:
	Server();
	~Server();


	VOID Initialize();
	VOID Communicate();



	bool Receive(SOCKET& sock, HEADER& header, std::vector<char>& buffer);
	bool ReceivePacketData(SOCKET& sock, HEADER& header, std::vector<char>& buffer, short& packetLegnth);
	bool SendAll(std::vector<char>& buffer, short packetLength);

	void Disconnect(SOCKET& sock);
	static DWORD WINAPI SocketThread(LPVOID lpParam);

};

