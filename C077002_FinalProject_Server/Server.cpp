#include "Server.h"

Server::Server() :clientSockets(std::vector<SOCKET>())
{
	SERVERPORT = PORT;
	//뮤텍스 생성
	hMutex = CreateMutex(NULL, FALSE, NULL);
	if (hMutex == NULL) {
		Error_Quit(_T("CreateMutex()"));
	}
	if (WSAStartup(MAKEWORD(2, 2), &Wsadata) != 0)
		return;
}
Server::~Server()
{
	//CloseSocket()
	closesocket(Listen_Socket);

	//WIndsock Quit
	WSACleanup();

	// 뮤텍스 종료
	CloseHandle(hMutex);

}

//Displaying Socket Error
VOID Server::Error_Quit(const TCHAR* Msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, Msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

//Displaying Socket Function Error
VOID Server::Error_Display(const TCHAR* Msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << "[" << Msg << "]" << (TCHAR*)lpMsgBuf;
	LocalFree(lpMsgBuf);
}



//accept()
bool Server::Accept() {
	AddressLen = sizeof(clientAddress);
	clientSocket = accept(Listen_Socket, (SOCKADDR*)&clientAddress, &AddressLen);
	if (clientSocket == INVALID_SOCKET)
	{
		Error_Display(_T("Accept\n"));
		return false;
	}
	return true;
}


/// <summary>
/// 멀티스레드 기본 세팅하기(소켓 생성, 바인드, 리슨)
/// 
/// </summary>
VOID Server::Initialize()
{
	INT retval = 0;

	// 소켓 생성
	Listen_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (Listen_Socket == INVALID_SOCKET)
		Error_Quit(_T("Socket()\n"));

	// Bind
	ZeroMemory(&ServerAddress, sizeof(SOCKADDR_IN));
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	ServerAddress.sin_port = htons(SERVERPORT);
	retval = bind(Listen_Socket, (SOCKADDR*)&ServerAddress, sizeof(ServerAddress));
	if (retval == SOCKET_ERROR)
		Error_Quit(_T("Bind()\n"));

	
	// listen
	retval = listen(Listen_Socket, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		Error_Quit(_T("listen()\n"));
}

/// <summary>
/// 클라이언트 연결 체크
/// </summary>
VOID Server::Communicate()
{
	HANDLE hThread = NULL;

	while (1)
	{
		// 클라이언트 accept 실패시 종료
		if (!Accept()) {
			break;
		} 

		//Displaying Client Display
		std::wcout << std::endl << _T("[TCP Server] Client Connected : IP Address = ") << inet_ntoa(clientAddress.sin_addr) << _T(", Port = ") << ntohs(clientAddress.sin_port) << std::endl;

		//Create Thread
		hThread = CreateThread(NULL, 0, SocketThread, this, 0, NULL);
		if (hThread == NULL)
			closesocket(clientSocket);
		else {
			CloseHandle(hThread);
		}


	}


}

/// <summary>
/// 클라이언트 스레드
/// 클라이언트 스레드는 각각 버퍼를 가지고 있는다.
/// </summary>
/// <param name="lpParam">서버 포인터</param>
/// <returns>DWORD</returns>
DWORD WINAPI Server::SocketThread(LPVOID lpParam)
{
	Server* This = (Server*)lpParam;
	SOCKET sock = This->clientSocket;
	INT Retval = 0;
	INT addressLen;
	SOCKADDR_IN threadSocketAddress;

	//Get Client Information
	addressLen = sizeof(SOCKADDR_IN);
	getpeername(sock, (SOCKADDR*)&threadSocketAddress, &addressLen);

	// 수신받은 데이터 출력용 버퍼
	std::vector<char> buffer;
	while (1)
	{
		HEADER header;
		if (This->Receive(sock, header, buffer)) {
			This->SendAll(buffer, buffer.size());
		}
		else {
			This->Disconnect(sock);
			This->SendAll(buffer, buffer.size());
			break;
		}

	}
	closesocket(sock);
	return 1;
}

/// <summary>
/// 접속해 있는 모든 클라이언트에게 데이터 송신 (Mutex를 통해 동기화 처리)
/// </summary>
/// <param name="buffer">송신할 버퍼</param>
/// <param name="packetLength">송신할 버퍼의 길이</param>
/// <returns>데이터 송신 성공 여부</returns>
bool Server::SendAll (std::vector<char>& buffer, short packetLength) {
	WaitForSingleObject(hMutex, INFINITE);
	for (int i = 0; i < this->clientSockets.size(); i++) {
		// 데이터 보내기

		int retval = send(this->clientSockets[i], reinterpret_cast<const char*>(buffer.data()), packetLength, 0);
		if (retval == SOCKET_ERROR) {
			this->Error_Display(_T("send()\n"));
			ReleaseMutex(hMutex);
			return false;
		}
	}
	ReleaseMutex(hMutex);
	return true;
}

/// <summary>
/// 클라이언트 연결 해제(클라이언트 목록에서 제거) -> 뮤텍스를 통해 동기화 처리
/// </summary>
/// <param name="sock">제거할 클라이언트 소켓</param>
void Server::Disconnect(SOCKET& sock) {
	WaitForSingleObject(hMutex, INFINITE);
	this->clientSockets.erase(
		remove(
			this->clientSockets.begin(),
			this->clientSockets.end(),
			sock),
		this->clientSockets.end());
	PrintClientCount();
	ReleaseMutex(hMutex);
}

/// <summary>
/// 데이터 수신타입에 따른 패킷 설정 함수(패킷헤더와 길이를 먼저 수신 후 헤더 타입에 따른 함수 실행)
/// </summary>
/// <param name="sock">수신 소켓</param>
/// <param name="header">헤더를 저장할 레퍼런스 변수</param>
/// <param name="buffer">수신받을 레퍼런스 버퍼</param>
/// <returns>수신 성공 여부</returns>
bool Server::Receive(SOCKET& sock, HEADER& header, std::vector<char>& buffer) {
	std::string str;
	short packetLegnth = 0;
	if (ReceivePacketData(sock, header, buffer, packetLegnth)) {
		switch (header) {
			// 연결 요청 헤더일 경우
			case HEADER::REQ_CON: {
				clientSockets.push_back(clientSocket);
				PrintClientCount();
				ConnectionPacket ConPacket = GetConnectionPacket(buffer, header, packetLegnth);
				str = GetCurrentTimeToString() + "Client: " + ConPacket.GetID() + " connected";
				std::cout << str << std::endl;
				SetConnectionPacket(buffer, ConPacket, HEADER::ACK_CON, str);
				break;
			}
			// 이동 요청 헤더일 경우
			case HEADER::REQ_MOVE: {
				MovePacket movePacket = GetMovePacket(buffer, header, packetLegnth);
				str = GetCurrentTimeToString() + "Client: " + std::string(1, movePacket.GetID()) + ", moved to  "
					+ std::to_string(movePacket.GetPosX()) + " "
					+ std::to_string(movePacket.GetPosY()) + " "
					+ std::to_string(movePacket.GetPosZ());
				std::cout << str << std::endl;
				SetMovePacket(buffer, movePacket, HEADER::ACK_MOVE, movePacket.GetPosX(), movePacket.GetPosY(), movePacket.GetPosZ(), str);
				break;
			}
			// 채팅 요청 헤더일 경우
			case HEADER::REQ_CHAT: {
				ChatPacket chatPacket = GetChatPacket(buffer, HEADER::ACK_CHAT, 0);
				str = "Client: " + std::string(1, chatPacket.GetID()) + " said: " + chatPacket.GetString();
				std::cout << str << std::endl;
				SetChatPacket(buffer, chatPacket, HEADER::ACK_CHAT, str);
				break;
			}
			// 연결 종료 헤더일 경우
			case HEADER::REQ_DIS: {
				ConnectionPacket ConPacket = GetConnectionPacket(buffer, HEADER::ACK_DIS, 0);
				str = GetCurrentTimeToString() + "Client: " + ConPacket.GetID() + " disconnected";
				std::cout << str << std::endl;
				SetConnectionPacket(buffer, ConPacket, HEADER::ACK_DIS, str);
				return false;
			}
		}
	}
	else {
		return false;
	}
	return true;
}

/// <summary>
/// 데이터 수신 함수(패킷 헤더와 길이를 먼저 받고, 그 후에 나머지 부분을 받음)
/// </summary>
/// <param name="sock">수신 소켓</param>
/// <param name="header">헤더를 저장할 레퍼런스 변수</param>
/// <param name="buffer">수신 버퍼</param>
/// <param name="packetLegnth">패킷 길이를 저장할 레퍼런스 변수</param>
/// <returns>패킷 데이터 수신 성공 여부</returns>
bool Server::ReceivePacketData(SOCKET& sock, HEADER& header, std::vector<char>& buffer, short &packetLegnth) {

	short offset = sizeof(header) + sizeof(packetLegnth);
	buffer.clear();
	buffer.resize(sizeof(header) + sizeof(packetLegnth));
	int retval = recv(sock, buffer.data(), sizeof(header) + sizeof(packetLegnth), 0);
	if (retval == SOCKET_ERROR)
	{
		this->Error_Display(_T("recv()\n"));
		return false;
	}
	memcpy(&header, buffer.data(), sizeof(header));
	memcpy(&packetLegnth, buffer.data() + sizeof(header), sizeof(packetLegnth));
	buffer.resize(packetLegnth);

	retval = recv(sock, buffer.data() + offset, packetLegnth, 0);
	if (retval == SOCKET_ERROR)
	{
		this->Error_Display(_T("recv()\n"));
		return false;
	}
	return true;
}

/// <summary>
/// 현재 시간 얻기
/// </summary>
/// <returns>string으로 변환한 현재 시간</returns>
std::string Server::GetCurrentTimeToString() const {
	time_t now1 = time(nullptr);
	tm tm_1;
	localtime_s(&tm_1, &now1);

	std::string str = std::to_string(tm_1.tm_year + 1900) + "년도 "
		+ std::to_string(tm_1.tm_mon + 1) + "월 "
		+ std::to_string(tm_1.tm_mday) +"일 "
		+ std::to_string(tm_1.tm_hour) + "시 "
		+ std::to_string(tm_1.tm_min) + "분 "
		+ std::to_string(tm_1.tm_sec) + "초 ";

	return str;
}

/// <summary>
/// 연결되어 있는 클라이언트의 수 출력
/// </summary>
void Server::PrintClientCount() const {
	std::cout << "Connected Client Counts: " << clientSockets.size() << std::endl;
}


// 패킷 관련
//-------------------------------------------------------------------------------------------------------
void Server::SetConnectionPacket(std::vector<char>& buffer, ConnectionPacket& ConPacket, HEADER header, std::string str) {
	ConPacket.SetHeader(header);
	ConPacket.SetString(str);
	ConPacket.SetLength();
	ConPacket.Serialize(buffer);
}

ConnectionPacket Server::GetConnectionPacket(std::vector<char>& buffer, HEADER header, short packetLength) {
	ConnectionPacket ConPacket = ConnectionPacket(header, 'a', packetLength, 0xffff);
	ConPacket.DeSerialize(buffer);
	return ConPacket;
}

void Server::SetMovePacket(std::vector<char>& buffer, MovePacket& movePacket, HEADER header, short x, short y, short z, std::string str) {
	movePacket.SetHeader(header);
	movePacket.SetPos(x, y, z);
	movePacket.SetString(str);
	movePacket.SetLength();
	movePacket.Serialize(buffer);
}

MovePacket Server::GetMovePacket(std::vector<char>& buffer, HEADER header, short packetLength) {
	MovePacket movePacket = MovePacket(header, 'a', packetLength, 0, 0, 0, 0xffff);
	movePacket.DeSerialize(buffer);
	return movePacket;
}

void Server::SetChatPacket(std::vector<char>& buffer, ChatPacket& chatPacket, HEADER header, std::string str) {
	chatPacket.SetHeader(header);
	chatPacket.SetString(str);
	chatPacket.SetLength();
	chatPacket.Serialize(buffer);
}

ChatPacket Server::GetChatPacket(std::vector<char>& buffer, HEADER header, short packetLength) {
	ChatPacket chatPacket = ChatPacket(header, 'a', packetLength, 0xffff);
	chatPacket.DeSerialize(buffer);
	return chatPacket;
}
//-------------------------------------------------------------------------------------------------------