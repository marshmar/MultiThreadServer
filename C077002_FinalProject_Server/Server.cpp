#include "Server.h"

Server::Server() :clientSockets(std::vector<SOCKET>())
{
	SERVERPORT = PORT;
	//���ؽ� ����
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

	// ���ؽ� ����
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
/// ��Ƽ������ �⺻ �����ϱ�(���� ����, ���ε�, ����)
/// 
/// </summary>
VOID Server::Initialize()
{
	INT retval = 0;

	// ���� ����
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
/// Ŭ���̾�Ʈ ���� üũ
/// </summary>
VOID Server::Communicate()
{
	HANDLE hThread = NULL;

	while (1)
	{
		// Ŭ���̾�Ʈ accept ���н� ����
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
/// Ŭ���̾�Ʈ ������
/// Ŭ���̾�Ʈ ������� ���� ���۸� ������ �ִ´�.
/// </summary>
/// <param name="lpParam">���� ������</param>
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

	// ���Ź��� ������ ��¿� ����
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
/// ������ �ִ� ��� Ŭ���̾�Ʈ���� ������ �۽� (Mutex�� ���� ����ȭ ó��)
/// </summary>
/// <param name="buffer">�۽��� ����</param>
/// <param name="packetLength">�۽��� ������ ����</param>
/// <returns>������ �۽� ���� ����</returns>
bool Server::SendAll (std::vector<char>& buffer, short packetLength) {
	WaitForSingleObject(hMutex, INFINITE);
	for (int i = 0; i < this->clientSockets.size(); i++) {
		// ������ ������

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
/// Ŭ���̾�Ʈ ���� ����(Ŭ���̾�Ʈ ��Ͽ��� ����) -> ���ؽ��� ���� ����ȭ ó��
/// </summary>
/// <param name="sock">������ Ŭ���̾�Ʈ ����</param>
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
/// ������ ����Ÿ�Կ� ���� ��Ŷ ���� �Լ�(��Ŷ����� ���̸� ���� ���� �� ��� Ÿ�Կ� ���� �Լ� ����)
/// </summary>
/// <param name="sock">���� ����</param>
/// <param name="header">����� ������ ���۷��� ����</param>
/// <param name="buffer">���Ź��� ���۷��� ����</param>
/// <returns>���� ���� ����</returns>
bool Server::Receive(SOCKET& sock, HEADER& header, std::vector<char>& buffer) {
	std::string str;
	short packetLegnth = 0;
	if (ReceivePacketData(sock, header, buffer, packetLegnth)) {
		switch (header) {
			// ���� ��û ����� ���
			case HEADER::REQ_CON: {
				clientSockets.push_back(clientSocket);
				PrintClientCount();
				ConnectionPacket ConPacket = GetConnectionPacket(buffer, header, packetLegnth);
				str = GetCurrentTimeToString() + "Client: " + ConPacket.GetID() + " connected";
				std::cout << str << std::endl;
				SetConnectionPacket(buffer, ConPacket, HEADER::ACK_CON, str);
				break;
			}
			// �̵� ��û ����� ���
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
			// ä�� ��û ����� ���
			case HEADER::REQ_CHAT: {
				ChatPacket chatPacket = GetChatPacket(buffer, HEADER::ACK_CHAT, 0);
				str = "Client: " + std::string(1, chatPacket.GetID()) + " said: " + chatPacket.GetString();
				std::cout << str << std::endl;
				SetChatPacket(buffer, chatPacket, HEADER::ACK_CHAT, str);
				break;
			}
			// ���� ���� ����� ���
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
/// ������ ���� �Լ�(��Ŷ ����� ���̸� ���� �ް�, �� �Ŀ� ������ �κ��� ����)
/// </summary>
/// <param name="sock">���� ����</param>
/// <param name="header">����� ������ ���۷��� ����</param>
/// <param name="buffer">���� ����</param>
/// <param name="packetLegnth">��Ŷ ���̸� ������ ���۷��� ����</param>
/// <returns>��Ŷ ������ ���� ���� ����</returns>
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
/// ���� �ð� ���
/// </summary>
/// <returns>string���� ��ȯ�� ���� �ð�</returns>
std::string Server::GetCurrentTimeToString() const {
	time_t now1 = time(nullptr);
	tm tm_1;
	localtime_s(&tm_1, &now1);

	std::string str = std::to_string(tm_1.tm_year + 1900) + "�⵵ "
		+ std::to_string(tm_1.tm_mon + 1) + "�� "
		+ std::to_string(tm_1.tm_mday) +"�� "
		+ std::to_string(tm_1.tm_hour) + "�� "
		+ std::to_string(tm_1.tm_min) + "�� "
		+ std::to_string(tm_1.tm_sec) + "�� ";

	return str;
}

/// <summary>
/// ����Ǿ� �ִ� Ŭ���̾�Ʈ�� �� ���
/// </summary>
void Server::PrintClientCount() const {
	std::cout << "Connected Client Counts: " << clientSockets.size() << std::endl;
}


// ��Ŷ ����
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