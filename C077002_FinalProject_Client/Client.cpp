#include "Client.h"

/// <summary>
/// 클라이언트 코드
/// 클라이언트는 멀티스레드로 Input과 Receive를 병렬로 처리한다.(비동기화)
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
/// 클라이언트 초기화(ID 생성, 소켓 생성, 연결 성공 시 패킷 송신)
/// </summary>
VOID Client::Initialize()
{
    // 아이디 생성
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
    
    // 연결 성공 시 패킷 송신
    std::vector<char> buffer = std::vector<char>();
    SetConnectionPacket(buffer, HEADER::REQ_CON, 0);
    Send(buffer, buffer.size());
    HEADER header;
    Receive(buffer, header);
}


/// <summary>
/// 서버와 데이터 통신 함수(멀티 스레드 사용, 비동기화 -> 동기화 처리하니까 Input 받을 동안 데이터를 받지 못해서 비동기화 처리)
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

    // 인풋 스레드 종료 기다리기
    WaitForMultipleObjects(2, hThreads,TRUE, INFINITE);

    // 스레드 종료하기
    for (HANDLE hThread : hThreads) {
        CloseHandle(hThread);
    }
}

/// <summary>
/// 인풋 스레드 함수
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

// 대기가능타이머로 1초마다 호출
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
/// 클라이언트 ID 생성 함수
/// </summary>
/// <param name="id"></param>
void Client::CreateID(char& id) {
    std::cout << "아이디를 입력하세요(A, B, C처럼 알파벳 한글자): " << std::endl;
    std::cin >> id;
}


/// <summary>
/// 데이터 송신 함수
/// </summary>
/// <param name="buffer">보낼 데이터 버퍼</param>
/// <param name="packetLength">보낼 길이</param>
/// <returns>데이터 송신 여부에 대한 boolean</returns>
bool Client::Send(std::vector<char>& buffer, short packetLength) {
    // 데이터 보내기
    int retval = send(sock, reinterpret_cast<const char*>(buffer.data()), packetLength, 0);
    if (retval == SOCKET_ERROR) {
        Error_Display(_T("send()"));
        return false;
    }
    return true;
}

/// <summary>
/// 데이터 수신 함수
/// </summary>
/// <param name="header">데이터 헤더</param>
/// <param name="buffer">버퍼</param>
/// <returns>데이터 수신 여부에 대한 boolean</returns>
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
/// 패킷 데이터를 수신 함수.
/// 패킷 헤더와 길이를 먼저 받고 버퍼를 패킷의 크기를 패킷의 길이만큼 재설정 한 후에 나머지 부분을 받아옴.
/// </summary>
/// <param name="header">헤더를 저장할 HEADER& 변수</param>
/// <param name="packetLength">길이를 저장할 short& 변수 </param>
/// <returns>패킷 데이터 수신 성공시: true, 실패 시: false</returns>
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
/// MovePacket 좌표 입력 
/// </summary>
/// <param name="x">x좌표</param>
/// <param name="y">y좌표</param>
/// <param name="z">z좌표</param>
void Client::InputPos(short& x, short& y, short& z) {
    std::cout << "x좌표를 입력하세요: ";
    std::cin >> x;
    std::cout << "y좌표를 입력하세요: ";
    std::cin >> y;
    std::cout << "z좌표를 입력하세요: ";
    std::cin >> z;
}

bool Client::InputClientProcess(std::vector<char>& buffer) {
    char input;
    std::string inputStr;
    short x = 0, y = 0, z = 0;
    while (true) {
        std::cout << "client의 행동을 결정하세요[m: move, s: sendMessage, p:printMessage,  q: quit]: \n";
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
            std::cout << "서버로 보낼 메시지를 입력하세요: ";
            inputStr.clear();
            std::cin >> inputStr;
            SetChatPacket(buffer, HEADER::REQ_CHAT, inputStr);
            return true;
        case 'q':
            SetConnectionPacket(buffer, HEADER::REQ_DIS, 0);
            Send(buffer, buffer.size());
            std::cout << "Client를 종료합니다...\n";
            return false;
        default:
            std::cout << "잘못된 입력입니다. 다시 입력해주세요. \n";
            continue;
        }
    }
}

// 패킷 관련
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
