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
	// 접속 클라이언트 소켓 배열
	std::vector<SOCKET> clientSockets;
	// 동기화 처리 뮤텍스
	HANDLE hMutex;

	VOID Error_Quit(const TCHAR* Msg);
	VOID Error_Display(const TCHAR* Msg);
	bool Accept();
	std::string GetCurrentTimeToString() const;
	void PrintClientCount() const;

	// 패킷 관련 함수들
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

