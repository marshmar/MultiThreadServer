#include "Client.h"

/// <summary>
/// Ŭ���̾�Ʈ �ڵ�
/// Ŭ���̾�Ʈ�� ��Ƽ������� Input�� Receive�� ���ķ� ó���Ѵ�.(�񵿱�ȭ)
/// </summary>

Client::Client()
{
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = -10000000LL;
    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (hTimer == NULL) {
        Error_Quit(_T("CreateWaitableTimer()"));
    }

    SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);

    if (WSAStartup(MAKEWORD(2, 2), &Wsadata) != 0)
        Error_Quit(_T("WSAStartup()"));
}

Client::~Client() {

    CloseHandle(hTimer);

    // closesocket()
    closesocket(sock);

    // Winsock Quit
    WSACleanup();

    std::vector<char> buffer = std::vector<char>();
    SetConnectionPacket(buffer, HEADER::REQ_DIS, 0);
    Send(buffer, buffer.size());
}
VOID Client::Error_Quit(const TCHAR* Msg) {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, Msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

VOID Client::Error_Display(const TCHAR* Msg) {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    std::wcout << "[" << Msg << "]" << (TCHAR*)lpMsgBuf;
    LocalFree(lpMsgBuf);
}



/// <summary>
/// Ŭ���̾�Ʈ �ʱ�ȭ(ID ����, ���� ����, ���� ���� �� ��Ŷ �۽�)
/// </summary>
VOID Client::Initialize()
{
    // ���̵� ����
    CreateID(id);

    INT retval;
    // socket()
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) Error_Quit(_T("socket()"));

    // connect
    ZeroMemory(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVERIP);
    serverAddress.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (retval == SOCKET_ERROR) Error_Quit(_T("connect()"));
    
    // ���� ���� �� ��Ŷ �۽�
    std::vector<char> buffer = std::vector<char>();
    SetConnectionPacket(buffer, HEADER::REQ_CON, 0);
    Send(buffer, buffer.size());
    HEADER header;
    Receive(buffer, header);
}


/// <summary>
/// ������ ������ ��� �Լ�(��Ƽ ������ ���, �񵿱�ȭ -> ����ȭ ó���ϴϱ� Input ���� ���� �����͸� ���� ���ؼ� �񵿱�ȭ ó��)
/// </summary>
VOID Client::Communicate() {
    HANDLE hThreads[2];

    //Create Input Thread
    hThreads[0] = CreateThread(NULL, 0, InputThreadProc, this, 0, NULL);
    if (hThreads[0] == NULL) {
        Error_Quit(_T("Create InputTread Error"));
    }
    //Create Receive Thread
    hThreads[1] = CreateThread(NULL, 0, ReceiveThreadProc, this, 0, NULL);
    if (hThreads[1] == NULL) {
        Error_Quit(_T("Create ReceiveThread Error"));
    }

    // ��ǲ ������ ���� ��ٸ���
    WaitForMultipleObjects(2, hThreads,TRUE, INFINITE);

    // ������ �����ϱ�
    for (HANDLE hThread : hThreads) {
        CloseHandle(hThread);
    }
}

/// <summary>
/// ��ǲ ������ �Լ�
/// </summary>
/// <param name="lpParam"></param>
/// <returns>DWORD</returns>
DWORD WINAPI Client::InputThreadProc(LPVOID lpParam) {
    Client* client = (Client*)lpParam;
    while (true) {
        std::vector<char> buffer = std::vector<char>();
        if (client->InputClientProcess(buffer)) {
            client->Send(buffer, buffer.size());
        }
        else {
            break;
        }        
    }
    return 1;
}

// ��Ⱑ��Ÿ�̸ӷ� 1�ʸ��� ȣ��
DWORD WINAPI Client::ReceiveThreadProc(LPVOID lpParam) {
    Client* client = (Client*)lpParam;

    while (true) {
        std::vector<char> buffer = std::vector<char>();
        HEADER header;
        if (!client->Receive(buffer, header))     
            break;
        WaitForSingleObject(client->hTimer, INFINITE);
    }
    return 1;
}

/// <summary>
/// Ŭ���̾�Ʈ ID ���� �Լ�
/// </summary>
/// <param name="id"></param>
void Client::CreateID(char& id) {
    std::cout << "���̵� �Է��ϼ���(A, B, Có�� ���ĺ� �ѱ���): " << std::endl;
    std::cin >> id;
}


/// <summary>
/// ������ �۽� �Լ�
/// </summary>
/// <param name="buffer">���� ������ ����</param>
/// <param name="packetLength">���� ����</param>
/// <returns>������ �۽� ���ο� ���� boolean</returns>
bool Client::Send(std::vector<char>& buffer, short packetLength) {
    // ������ ������
    int retval = send(sock, reinterpret_cast<const char*>(buffer.data()), packetLength, 0);
    if (retval == SOCKET_ERROR) {
        Error_Display(_T("send()"));
        return false;
    }
    return true;
}

/// <summary>
/// ������ ���� �Լ�
/// </summary>
/// <param name="header">������ ���</param>
/// <param name="buffer">����</param>
/// <returns>������ ���� ���ο� ���� boolean</returns>
bool Client::Receive(std::vector<char>& buffer, HEADER& header) {
    short packetLength = 0;
    if (ReceivePacketData(buffer, header, packetLength)) {
        switch (header) {
        case HEADER::ACK_CON: {
            ConnectionPacket ConPacket = GetConnectionPacket(buffer, header, packetLength);
            std::cout << ConPacket.GetString() << std::endl;
            break;
        }
        case HEADER::ACK_MOVE: {
            MovePacket movePacket = GetMovePacket(buffer, header, packetLength);
            std::cout << movePacket.GetString() << std::endl;
            break;
        }
        case HEADER::ACK_CHAT: {
            ChatPacket chatPacket = GetChatPacket(buffer, header, packetLength);
            std::cout << chatPacket.GetString() << std::endl;
            break;
        }
        case HEADER::ACK_DIS: {
            ConnectionPacket ConPacket = GetConnectionPacket(buffer, header, packetLength);
            std::cout << ConPacket.GetString() << std::endl;
            break;
        }
        }
    }
    else {
        return false;
    }
    return true;
}

/// <summary>
/// ��Ŷ �����͸� ���� �Լ�.
/// ��Ŷ ����� ���̸� ���� �ް� ���۸� ��Ŷ�� ũ�⸦ ��Ŷ�� ���̸�ŭ �缳�� �� �Ŀ� ������ �κ��� �޾ƿ�.
/// </summary>
/// <param name="header">����� ������ HEADER& ����</param>
/// <param name="packetLength">���̸� ������ short& ���� </param>
/// <returns>��Ŷ ������ ���� ������: true, ���� ��: false</returns>
bool Client::ReceivePacketData(std::vector<char>& buffer, HEADER& header, short& packetLength) {
    buffer.clear();
    buffer.resize(sizeof(header) + sizeof(packetLength));

    int retval = recv(sock, buffer.data(), sizeof(header) + sizeof(packetLength), 0);
    short offset = sizeof(header) + sizeof(packetLength);
    if (retval == SOCKET_ERROR)
    {
        this->Error_Display(_T("recv()"));
        return false;
    }
    memcpy(&header, buffer.data(), sizeof(header));
    memcpy(&packetLength, buffer.data() + sizeof(header), sizeof(short));

    if (packetLength <= 0) {
        this->Error_Display(_T("Invalid pakcet length"));
        return false;
    }

    buffer.resize(packetLength);

    retval = recv(sock, buffer.data() + offset, packetLength, 0);
    if (retval == SOCKET_ERROR)
    {
        this->Error_Display(_T("recv()"));
        return false;
    }
    return true;
}

/// <summary>
/// MovePacket ��ǥ �Է� 
/// </summary>
/// <param name="x">x��ǥ</param>
/// <param name="y">y��ǥ</param>
/// <param name="z">z��ǥ</param>
void Client::InputPos(short& x, short& y, short& z) {
    std::cout << "x��ǥ�� �Է��ϼ���: ";
    std::cin >> x;
    std::cout << "y��ǥ�� �Է��ϼ���: ";
    std::cin >> y;
    std::cout << "z��ǥ�� �Է��ϼ���: ";
    std::cin >> z;
}

bool Client::InputClientProcess(std::vector<char>& buffer) {
    char input;
    std::string inputStr;
    short x = 0, y = 0, z = 0;
    while (true) {
        std::cout << "client�� �ൿ�� �����ϼ���[m: move, s: sendMessage, p:printMessage,  q: quit]: \n";
        std::cin >> input;
        switch (input) {
        case 'm': 
            InputPos(x, y, z);
            SetMovePacket(buffer, HEADER::REQ_MOVE,
                x, y, z,
                "Client: " + std::string(1, id) + ", requested to move "
                + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z));
            std::cout << "buffer size: " + buffer.size() << std::endl;
            return true;
        case 's':
            std::cout << "������ ���� �޽����� �Է��ϼ���: ";
            inputStr.clear();
            std::cin >> inputStr;
            SetChatPacket(buffer, HEADER::REQ_CHAT, inputStr);
            return true;
        case 'q':
            SetConnectionPacket(buffer, HEADER::REQ_DIS, 0);
            Send(buffer, buffer.size());
            std::cout << "Client�� �����մϴ�...\n";
            return false;
        default:
            std::cout << "�߸��� �Է��Դϴ�. �ٽ� �Է����ּ���. \n";
            continue;
        }
    }
}

// ��Ŷ ����
void Client::SetConnectionPacket(std::vector<char>& buffer, HEADER header, short packetLength) {
    // Send Connection Packet
    ConnectionPacket ConPacket = ConnectionPacket(header, id, packetLength, 0xffff);
    ConPacket.SetLength();
    ConPacket.Serialize(buffer);
}

ConnectionPacket Client::GetConnectionPacket(std::vector<char>& buffer, HEADER header, short packetLength){
    ConnectionPacket ConPacket = ConnectionPacket(header, id, packetLength, 0xffff);
    ConPacket.DeSerialize(buffer);
    return ConPacket;
}

void Client::SetMovePacket(std::vector<char>& buffer, HEADER header, short x, short y, short z, std::string str) {
    MovePacket movePacket = MovePacket(header, id, 0, x, y, z, 0xffff);
    movePacket.SetString(str);
    movePacket.SetLength();
    movePacket.Serialize(buffer);
}

MovePacket Client::GetMovePacket(std::vector<char>& buffer, HEADER header, short packetLength) {
    MovePacket movePacket = MovePacket(header, id, packetLength, 0, 0, 0, 0xffff);
    movePacket.DeSerialize(buffer);
    return movePacket;
}

void Client::SetChatPacket(std::vector<char>& buffer, HEADER header, std::string str) {
    ChatPacket chatPacket = ChatPacket(header, id, 0, 0xffff);
    chatPacket.SetString(str);
    chatPacket.SetLength();
    chatPacket.Serialize(buffer);
}

ChatPacket Client::GetChatPacket(std::vector<char>& buffer, HEADER header, short packetLength) {
    ChatPacket chatPacket = ChatPacket(header, id, packetLength, 0xffff);
    chatPacket.DeSerialize(buffer);
    return chatPacket;
}
