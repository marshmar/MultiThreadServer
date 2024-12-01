#pragma once

#define _CRT_SECURE_NO_WARNINGS         // 최신 VC++ 컴파일 시 경고 방지
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지

#pragma comment(lib, "ws2_32")
#include <Winsock2.h>
#include<stdlib.h>
#include<iostream>
#include<tchar.h>
#include<time.h>

#include "ConnectionPacket.h"
#include "MovePacket.h"
#include "ChatPacket.h"



#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
class Client
{
private:
    // 소켓 함수 오류 출력 후 종료
    VOID Error_Quit(const TCHAR* Msg);
    // 소켓 함수 오류 출력
    VOID Error_Display(const TCHAR* Msg);
    WSADATA Wsadata;
    SOCKET sock;
    SOCKADDR_IN serverAddress;
    // 클라이언트 id
    char id;
    HANDLE hTimer;

    void CreateID(char& id);
    void InputPos(short& x, short& y, short& z);
    void SetConnectionPacket(std::vector<char>& buffer, HEADER header, short packetLength);
    ConnectionPacket GetConnectionPacket(std::vector<char>& buffer, HEADER header, short packetLength);
    void SetMovePacket(std::vector<char>& buffer, HEADER header, short x, short y, short z, std::string str);
    MovePacket GetMovePacket(std::vector<char>& buffer, HEADER header, short packetLength);
    void SetChatPacket(std::vector<char>& buffer, HEADER header, std::string str);
    ChatPacket GetChatPacket(std::vector<char>& buffer, HEADER header, short packetLength);

public:
    Client();
    ~Client();

    bool Send(std::vector<char>& buffer, short packetLength);
    bool Receive(std::vector<char>& buffer, HEADER& header);
    bool ReceivePacketData(std::vector<char>& buffer, HEADER& header,short& packetLength);
    VOID Initialize();
    VOID Communicate();

    bool InputClientProcess(std::vector<char>& buffer);
    static DWORD WINAPI InputThreadProc(LPVOID lpParam);
    static DWORD WINAPI ReceiveThreadProc(LPVOID lpParam);


};

