// SocketAPI.cpp : 정적 라이브러리를 위한 함수를 정의합니다.
//

#include "pch.h"
#include "framework.h"

#include "SocketAPI.h"


// 디버그 출력
//OutputDebugString(L"ddd");



// ExchangeManager 클래스 메서드들

// 멤버 변수 초기화
ExchangeManager::ExchangeManager()
{
	mp_data = nullptr;
	m_total_size = 0;
	m_current_size = 0;
}

// 전송, 수신을 위해 할당된 메모리 제거
ExchangeManager::~ExchangeManager()
{
	DeleteData();
}

// 전송, 수신에 사용할 메모리 할당 (a_data_size에 할당할 크기를 명시하면 할당된 메모리 주소 반환)
char* ExchangeManager::MemoryAlloc(int a_data_size)
{
	TR("ExchangeManager::MemoryAlloc - 전송, 수신에 사용할 메모리 할당\n");

	// 기존에 사용하던 메모리(m_total_size)와 현재 필요한 메모리 크기(a_data_size)가 동일하면 다시 메모리를 할당할 필요가 없다
	if (m_total_size != a_data_size)
	{
		if (mp_data != nullptr) delete[] mp_data; // 이미 할당된 메모리가 있으면 제거
		mp_data = new char[a_data_size]; // 필요한 만큼 동적 메모리 할당
		m_total_size = a_data_size; // 할당된 크기 기억
	}

	m_current_size = 0; // 작업 위치를 가장 첫 위치로 초기화
	
	return mp_data; // 할당된 메모리의 시작 위치 반환
}

// 전송, 수신에 사용된 메모리 제거
void ExchangeManager::DeleteData()
{	
	TR("ExchangeManager::DeleteData - 전송, 수신에 사용된 메모리 제거\n");

	if (mp_data != nullptr) // 할당된 메모리 제거, 작업과 관련된 변수들 초기화
	{
		delete[] mp_data;
		mp_data = nullptr;
		m_total_size = 0;
	}
}





// SendManager 클래스 메서드들 (전송 측 클래스)

// 전송할 위치와 크기 계산 (전송 시작 위치 변경) (큰 크기의 데이터를 나눌때 사용)
int SendManager::GetPosition(char** ap_data, int a_data_size) // a_data_size : default 2048
{
	TR("SendManager::GetPosition - 전송할 위치와 크기 계산 (큰 크기의 데이터를 나눌때 사용)\n");
	
	// 전송할 데이터를 하나의 메모리로 관리하고 메모리 시작 위치부터 2048byte (a_data_size) 단위로 데이터를 전송
	// 변경된 시작 위치는 ap_data에 넣는다, 실제 전송해야할 데이터 크기를 함수 반환값으로 알려준다
	// ex) 전송할 데이터의 전체 크기가 2050이면 GetPosition 함수를 처음 사용했을 때는 2048반환, 두 번째 호출때는 2반환
	
	// **ap_data : 전송할 메모리의 시작 주소를 받아야 하는데 주소를 받아오려면 포인터의 주소를 넘겨야 하기 때문에 2차원 포인터 사용
	*ap_data = mp_data + m_current_size; // 새로운 전송 위치에 대한 주소를 첫번째 인자에 저장 

	// 전송 크기를 계산하기 위해 2048byte(a_data_size)를 더한 크기가 최대 크기보다 작은지 체크
	if (m_current_size + a_data_size < m_total_size)
	{	
		// 최대 크기보다 작은 경우 2048byte (a_data_size) 만큼 전송하면 된다
		m_current_size += a_data_size; // 다음 위치를 계산할 수 있도록 전송한 총 크기를 구한다
	}
	else
	{
		// 2048byte 보다 작은 경우, 실제로 남은 크기만 전송한다
		a_data_size = m_total_size - m_current_size;
		m_current_size = m_total_size; // 현재 위치를 마지막 위치로 옮긴다 (이번이 마지막 전송임)
	}

	return a_data_size; // 계산된 전송크기를 반환
}





// RecvManager 클래스 메서드들 (수신 측 클래스)

// 수신된 데이터를 기존에 수신된 데이터에 추가
int RecvManager::AddData(char* ap_data, int a_size)
{	
	TR("RecvManager::AddData - 수신된 데이터를 기존에 수신된 데이터에 추가\n");

	// m_current_size: 일정 크기의 데이터가 수신될 때마다 저장할 위치를 계산하기 위해 현재 수신된 데이터의 총 크기를 기억할 변수
	memcpy(mp_data + m_current_size, ap_data, a_size); // 기존 수신 데이터(mp_data + m_current_size)위치에 새로 수신된 데이터(ap_data) 를 복사 (a_size만큼)
	m_current_size += a_size; // 총 수신 크기 계산
	return m_current_size;    // 현재 수신된 데이터의 크기를 반환
}





// Socket 클래스 메서드들

// 객체 생성시에 프로토콜 구분 값과 데이터 수신 및 연결 해제에 사용할 메시지 ID 지정
Socket::Socket(unsigned char a_valid_key, int a_data_notify_id)
{
	m_valid_key = a_valid_key;                   // 사용자가 지정한 프로토콜 구분 값 저장
	mp_send_data = new char[8192];               // 전송용으로 사용할 메모리 할당 (8kbyte)
	mp_recv_data = new char[8192];               // 수신용으로 사용할 메모리 할당 (8kbyte)

	*(unsigned char*)mp_send_data = a_valid_key; // 전송할 메모리의 선두 1바이트에 구분값을 넣는다	

	mh_notify_wnd = nullptr;                     // 데이터 수신 및 연결 해제 메시지를 받을 윈도우 핸들 값 초기화
	m_data_notify_id = a_data_notify_id;         // 데이터 수신 및 연결 해제 메시지 ID로 사용할 값 (FD_READ, FD_CLOSE)

	// 소켓 시스템을 사용하도록 설정 
	WSADATA temp;
	WSAStartup(0x0202, &temp);
}


Socket::~Socket()
{
	// 전송과 수신에 사용하던 메모리 제거
	delete[] mp_send_data;
	delete[] mp_recv_data;

	// 소켓 시스템을 더 이상 사용하지 않도록 설정
	WSACleanup();
}


// 데이터 전송 함수 (전달된 정보를 가지고 mp_send_data 메모리에 약속된 Head 정보를 구성해서 전송) (return -> 성공:1, 실패:0)
int Socket::SendFrameData(SOCKET ah_socket, unsigned char a_message_id, const char* ap_body_data, BS a_body_size)
{
	TR("Socket::SendFrameData - 데이터 전송 함수 (전달된 정보를 가지고 mp_send_data 메모리에 약속된 Head 정보를 구성해서 전송)\n");
	
	// 메시지 ID를 두 번째 바이트에 저장 (첫 번째 바이트에는 구분값이 이미 들어있다 -> 객체 생성자)
	*(unsigned char*)(mp_send_data + 1) = a_message_id;
	
	// 세번째와 네번째 바이트에(2byte) 'Body'데이터의 크기 저장 
	*(BS*)(mp_send_data + 2) = a_body_size;

	// 다섯번째 위치에 (HEAD_SIZE 4byte 다음 위치) 'Body'를 구성하는 데이터 복사 
	memcpy(mp_send_data + HEAD_SIZE, ap_body_data, a_body_size);
	
	// 소켓으로 데이터 전송. 전송 총량은 Head + Body
	// send() 함수의 반환 값 : 전송된 총 바이트수
	// (ah_socket) 소켓으로 (mp_send_data) 데이터를 (HEAD_SIZE + a_body_size) 크기만큼 보내겠다
	if (send(ah_socket, mp_send_data, HEAD_SIZE + a_body_size, 0) == HEAD_SIZE + a_body_size)
	{
		TR("Socket::SendFrameData - 전송 성공\n");
		return 1; // 전송 성공
	}
	else
	{
		TR("Socket::SendFrameData - 전송 실패\n");
		return 0; // 전송 실패	
	}

}


// 안정적인 데이터 수신 (재시도 수신) (return -> 성공:1, 실패:0)
int Socket::ReceiveData(SOCKET ah_socket, BS a_body_size)
{
	TR("Socket::ReceiveData - 안정적인 데이터 수신 (재시도 수신)\n");

	// 수신되는 데이터가 크거나 네트워크 상태가 좋지 못하면 데이터가 여러 번에 걸쳐 나누어져 수신될 수 있다
	// 정확한 수신을 위해 재시도 읽기를 해야 함 (반복문 사용)
	
	BS total_size = 0; // 총 읽은 데이터
	int retry = 0;     // 재시도 횟수
	int read_size;     // recv 함수의 반환값 (recv: 실제로 읽은 데이터의 크기 값을 반환)

	// a_body_size에 지정한 크기만큼 (전송된 데이터 크기만큼) 한번에 수신하지 못 할 수 있으므로 반복문을 이용해 여러번 재시도하며 읽는다
	while (total_size < a_body_size)
	{
		// 실제로 읽은 크기를 제외한 나머지 크기를 계속 반복해서 읽는다 (데이터를 읽을 위치를 읽은 크기만큼 이동하며)
		// ah_socket으로 (mp_recv_data + total_size) 위치부터 (a_body_size - total_size) 만큼 데이터를 읽는다
		read_size = recv(ah_socket, mp_recv_data + total_size, a_body_size - total_size, 0); 

		if (read_size == SOCKET_ERROR) // 수신중에 에러가 발생하면 SOCKET_ERROR 반환
		{
			// 데이터 수신도중 오류가 난 경우 0.5초동안 10회 반복하면서 재시도
			retry++;
			Sleep(50); // 50ms 지연 시킨다
			if (retry > 10) break;
		}
		else // 정상적으로 데이터 수신
		{
			// 실제로 수신된 크기를 total_size에 합산
			if (read_size > 0) total_size += (BS)read_size;
			
			// 총 수신된 크기가 a_body_size보다 작으면 (데이터를 한번에 읽지 못했으면) 수신 상태가 좋지 않다는 뜻, 약간의 지연을 주면서 다시 읽기를 시도
			if (total_size < a_body_size) Sleep(5);

			retry = 0; // 재시도 횟수 초기화
		}
	}

	if (retry <= 10)
	{
		TR("Socket::ReceiveData - 데이터 수신 성공\n");
		return 1; // 수신 성공
	}
	else
	{
		TR("Socket::ReceiveData - 데이터 수신 실패\n");
		return 0; // 수신 실패
	}

	//return retry <= 10; // 0이면 수신 실패, 1이면 수신 성공
}


// 데이터가 수신되었을 때 수신된 데이터를 처리하는 함수
// 데이터 수신중 오류가 발생해 DisconnectSocket을 호출하면 0을 반환, 정상적으로 처리하면 1반환 
int Socket::ProcessRecvEvent(SOCKET ah_socket)
{
	TR("Socket::ProcessRecvEvent - 데이터가 수신되었을 때 수신된 데이터를 처리하는 함수\n");
	
	// 수신된 데이터를 'Head'와 'Body'로 분리하고 'Head'정보를 분석해서 'MessageID'에 따른 작업을 할 수 있도록 ProcessRecvData 함수를 호출
	// 수신하는 과정에서 끊어 읽기, 재시도 읽기에 의해 FD_READ 이벤트가 과도하게 발생할 수 있으므로
	// 비동기 처리 (WSAASyncSelect)를 이용해 FD_READ 이벤트가 추가로 발생하지 않도록 설정
	// 수신 작업 완료후 FD_READ 이벤트를 수신할 수 있도록 비동기를 다시 걸어준다
	
	unsigned char msg_id; // msg_id : Body에 저장된 데이터의 종류를 구분 (message_id)
	BS body_size;

	// FD_READ 이벤트가 과도하게 발생하지 않도록 제거 (FD_CLOSE에 의해서만 m_data_notify_id 이벤트가 발생하도록 설정)
	// 이 소켓(ah_socket)에 FD_CLOSE가 발생하면 이 윈도우(mh_notify_wnd)에게 m_data_notify_id 메시지를 준다
	WSAAsyncSelect(ah_socket, mh_notify_wnd, m_data_notify_id, FD_CLOSE); // WSAAsyncSelect : 특정 소켓에 비동기를 건다

	unsigned char key = 0;              // key: 구분값. 이 프로토콜이 정상적인 프로토콜인지 체크 (1byte)
	recv(ah_socket, (char*)&key, 1, 0); // 구분값 수신 (ah_socket으로부터 1바이트의 데이터를 key에다가 가져옴)
	
	// 사용자가 지정한 구분값과 일치하는지 체크	
	if (key == m_valid_key)
	{
		TR("Socket::ProcessRecvEvent - 정상적인 프로토콜\n");

		recv(ah_socket, (char*)&msg_id, 1, 0);             // Message ID 수신 (1byte)
		recv(ah_socket, (char*)&body_size, sizeof(BS), 0); // Body size 수신  (2byte)

		if (body_size > 0)
		{
			// Body size는 값이 크기 때문에 안정적인 재시도 수신이 가능한 ReceiveData 함수로 데이터를 수신한다
			if (!ReceiveData(ah_socket, body_size)) // ReceiveData : 데이터 수신 실패하면 0반환
			{
				// 데이터를 수신하다 오류가 발생한 경우 연결된 소켓을 해제
				// DisconnectSocket 함수는 상속받은 자식 클래스에서 오버라이딩 으로 재정의하여 자신들이 원하는 작업을 추가할 함수
				DisconnectSocket(ah_socket, -2);
				return 0;
			}
		}
		
		// 정상적으로 'Head'와 'Body'를 수신한 경우 이 정보들을 사용하여 사용자가 원하는 작업을 처리
		// ProcessRecvData 함수는 상속받은 자식 클래스에서 오버라이딩 으로 재정의하여 자신들이 원하는 작업을 추가할 함수
		if (ProcessRecvData(ah_socket, msg_id, mp_recv_data, body_size) == 1)
		{
			// 소켓에 문제가 발생하지 않았다면 다시 수신 이벤트 처리가 가능하도록 FD_READ 옵션을 추가 (비동기 재설정)
			WSAAsyncSelect(ah_socket, mh_notify_wnd, m_data_notify_id, FD_READ | FD_CLOSE);
		}

	}
	else // 구분값이 잘못된 경우, 접속 해체
	{
		TR("Socket::ProcessRecvEvent - 잘못된 프로토콜\n");
		DisconnectSocket(ah_socket, -1);
		return 0;
	}

	return 1; // 정상적으로 처리함
}


// ASCII 형식의 문자열을 유니코드로 변환
void Socket::AsciiToUnicode(wchar_t* ap_dest_ip, char* ap_src_ip)
{
	int ip_length = strlen(ap_src_ip) + 1;
	memset(ap_dest_ip, 0, ip_length << 1);

	// 1바이트 형식으로 되어 있는 문자열을 2바이트 형식으로 변경
	for (int i = 0; i < ip_length; i++)
		ap_dest_ip[i] = ap_src_ip[i];
}

// 유니코드 형식의 문자열을 ASCII로 변환
void Socket::UnicodeToAscii(char* ap_dest_ip, wchar_t* ap_src_ip)
{
	int ip_length = wcslen(ap_src_ip) + 1; // wcslen : strlen 의 유니코드 버전
		
	// 2바이트 형식으로 되어 있는 문자열을 1바이트 형식으로 변경
	for (int i = 0; i < ip_length; i++)
		ap_dest_ip[i] = (char)ap_src_ip[i];
}




	
// UserData 클래스 메서드들

// 멤버변수 초기화, 전송과 수신에 사용할 객체 생성
UserData::UserData()
{
	mh_socket = INVALID_SOCKET; // 소켓 핸들 초기화
	m_ip_address[0] = 0; // 주소 값 초기화
	mp_send_man = new SendManager(); // 전송용 객체 생성
	mp_recv_man = new RecvManager(); // 수신용 객체 생성
}


// 사용하던 소켓 제거, 전송과 수신에 사용한 객체 제거
// 파괴자 : 가상으로 선언
UserData::~UserData()
{	
	// 소켓이 생성되어 있다면 소켓을 제거
	if (mh_socket != INVALID_SOCKET) closesocket(mh_socket);

	// 전송과 수신을 위해 생성했던 객체 제거
	delete mp_send_man;
	delete mp_recv_man;
}


// 연결된 소켓을 닫고 초기화 (a_linger_flag가 0이 아니면 소켓을 즉시 닫는다)
// 제거하고자 하는 소켓으로 데이터가 수신 중이면 수신이 완료될 때 까지 제거를 못하기 때문에 소켓을 즉시 닫고 싶으면 링거 옵션 사용
void UserData::CloseSocket(int a_linger_flag)
{
	TR("UserData::CloseSocket - 연결된 소켓을 닫고 초기화\n");

	if (mh_socket != INVALID_SOCKET) // 소켓이 생성되어 있다면
	{
		if (a_linger_flag == 1)
		{
			TR("UserData::CloseSocket - 소켓을 즉시 닫는다 (LINGER)\n");
			LINGER temp_linger = { TRUE, 0 }; // 데이터가 송수신되는 것과 상관없이 소켓을 바로 닫겠다
			setsockopt(mh_socket, SOL_SOCKET, SO_LINGER, (char*)&temp_linger, sizeof(temp_linger)); // mh_socket소켓의 LINGER 옵션 변경
		}
		
		closesocket(mh_socket); // 소켓을 닫는다
		mh_socket = INVALID_SOCKET; // 소켓 핸들 초기화
	}
}





// ServerSocket 클래스 메서드들


// a_valid_key: 프로토콜 구분값, a_max_user_count: 서버가 관리할 최대 사용자 수, ap_user_data: 데이터 객체(UserData 객체 or UserAccount 객체)
// a_accept_notify_id: FD_ACCEPT 발생시 윈도우에 전달할 메시지 ID, a_data_notify_id: FD_READ, FD_CLOSE 발생시 윈도우에 전달할 메시지 ID
ServerSocket::ServerSocket(unsigned char a_valid_key, unsigned short a_max_user_count, UserData* ap_user_data,
	int a_accept_notify_id, int a_data_notify_id) : Socket(a_valid_key, a_data_notify_id)
{
	m_max_user_count = a_max_user_count;     // 서버에 접속할 최대 사용자 수
	m_accept_notify_id = a_accept_notify_id; // 새로운 사용자가 접속했을 때 발생할 메시지 ID를 저장 (FD_ACCEPT)
	mh_listen_socket = INVALID_SOCKET;       // Listen 작업용 소켓 초기화

	// 최대 사용자 수만큼 사용자 정보를 저장할 객체를 관리할 포인터 생성 (UserData** mp_user_list)
	mp_user_list = new UserData * [m_max_user_count]; // UserData 객체 또는 UserAccount 객체를 담을수 있다 (다형성)

	// 최대 사용자 수만큼 사용자 정보를 저장할 객체를 개별적으로 생성
	for (int i = 0; i < m_max_user_count; i++)
	{
		// 다형성 사용시에 특정 클래스 형식에 종속되지 않도록 멤버함수를 사용해서 객체를 생성
		// CreateObject 함수는 매개변수로 전달된 ap_user_data와 동일한 객체를 생성한다 (UserData 객체 or UserAccount 객체)
		mp_user_list[i] = ap_user_data->CreateObject();
	}

	// 사용자 정보를 생성하기 위해 매개변수로 전달받은 객체를 제거한다
	// 이미 동일한 형식의 객체가 mp_user_list에 할당되어 있기 때문에 보관할 필요가 없다
	delete[] ap_user_data;
}


ServerSocket::~ServerSocket()
{
	// listen 소켓이 생성되어 있다면 제거
	if (mh_listen_socket != INVALID_SOCKET) closesocket(mh_listen_socket);

	// 최대 사용자 수만큼 생성되어 있던 객체를 제거
	for (int i = 0; i < m_max_user_count; i++)
		delete mp_user_list[i];

	// 사용자 객체를 관리하기 위해 생성했던 포인터를 제거
	delete[] mp_user_list;
}


// 서버 서비스의 시작 (socket - bind - listen)
// StartServer 함수를 사용하면 클라이언트가 접속할 수 있는 상태가 된다
// return -> mh_listen_socket 생성 실패: -1, bind 실패: -2, 성공: 1
int ServerSocket::StartServer(const wchar_t* ap_ip_address, int a_port, HWND ah_notify_wnd)
{
	TR("ServerSocket::StartServer - 서버 서비스의 시작 (socket - bind - listen)\n");
	
	// 비동기 형식의 소켓에 이벤트(FD_ACCEPT, FD_READ, FD_CLOSE)가 발생했을 때 전달되는 메시지를 수신할 윈도우의 핸들을 저장
	mh_notify_wnd = ah_notify_wnd;

	// 클라이언트의 접속을 받아주는 listen 작업용 소켓을 TCP 형식으로 생성 (AF_INET: 주소체계, SOCK_STREAM: TCP)
	mh_listen_socket = socket(AF_INET, SOCK_STREAM, 0);

	// 소켓 생성에 성공했는지 체크
	if (mh_listen_socket < 0) 
	{
		TR("ServerSocket::StartServer - listen socket 생성 실패\n");
		return -1; // 소켓 생성 실패!!
	}
	
	// 유니코드 형식으로 전달된 IP주소를 ASCII 형식으로 변경
	char temp_ip_address[16];
	UnicodeToAscii(temp_ip_address, (wchar_t*)ap_ip_address);


	// 네트워크 장치에 mh_listen_socket을 연결하기 위해 IP주소와 포트번호를 가지고 기본 정보를 구성
	// 소켓의 네트워크 설정 (주소 체계, IP, 포트번호)
	sockaddr_in serv_addr;
	memset((char*)&serv_addr, 0, sizeof(serv_addr));        // serv_addr을 0으로 초기화
	serv_addr.sin_family = AF_INET;                         // 주소 체계 설정
	serv_addr.sin_addr.s_addr = inet_addr(temp_ip_address); // 소켓이 연결할 네트워크 카드 설정 (IP)
	serv_addr.sin_port = htons((unsigned short)a_port);     // 포트번호 설정


	// 네트워크 장치에 mh_listen_socket을 연결한다 (bind : 서버 소켓 바인딩 - 네트워크 카드에 소켓 연결)
	// mh_listen_socket을 serv_addr 네트워크 정보를 가진 네트워크 카드에 연결
	if (bind(mh_listen_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
	{	
		TR("ServerSocket::StartServer - bind 실패\n");

		// 실패한 경우 소켓을 제거하고 mh_listen_socket 변수를 초기화
		closesocket(mh_listen_socket);
		mh_listen_socket = INVALID_SOCKET;
		return -2; // bind 오류!!
	}

	// 클라이언트 접속을 허락한다 (이 시점부터 클라이언트의 접속이 가능) 
	// listen() : 클라이언트의 연결 요청 대기
	listen(mh_listen_socket, 5); // 5: 대기자수 (여러 클라이언트가 동시에 접속하더라도 한번에 5개만 처리)
	TR("ServerSocket::StartServer - 클라이언트 연결 요청 대기 (listen)\n");


	// accept : 클라이언트 연결 수립 (실제 클라이언트의 접속)
	// accept 함수를 바로 호출하면 새로운 사용자가 접속할 때 까지 프로그램이 응답없음에 빠지기 때문에 비동기 설정 (WSAAsyncSelect)
	// mh_listen_socket에 클라이언트가 접속하려고 한다면(FD_ACCEPT) ah_notify_wnd 윈도우 에게 m_accept_notify_id 메시지를 준다
	WSAAsyncSelect(mh_listen_socket, ah_notify_wnd, m_accept_notify_id, FD_ACCEPT);

	return 1; // listen 작업 성공

	// 만약, '192.168.0.77'번에서 29999번 포트로 서버 서비스를 시작하고 싶다면 아래와 같이 StartServer 함수를 사용
	// my_server.StartServer(L"192.168.0.77", 29999, m_hWnd);
	// 새로운 사용자가 접속할 때 마다 m_hWnd 핸들에 해당하는 윈도우로 m_accept_notify_id에 저장된 윈도우 메시지가 발생한다
}


// 클라이언트의 접속 처리 (FD_ACCEPT 처리 함수)
// 서버가 listen 작업을 수행하여 서비스가 시작되면 새로운 클라이언트가 접속할 때마다 FD_ACCEPT 이벤트가 발생하고
// 윈도우에 메시지 전달 -> 전달된 메시지 처리 (ProcessToAccept)
// wParam: 메시지가 발생하게된 소켓의 핸들 (mh_listen_socket)
// lParam: 소켓에 에러가 있는지(WSAGETSELECTERROR) or 어떤 이벤트 때문에 발생했는지(WSAGETSELECTEVENT)
// return -> h_client_socket 생성 실패: -1, 최대 접속인원 초과: -2, 성공: 1
int ServerSocket::ProcessToAccept(WPARAM wParam, LPARAM lParam)
{
	TR("ServerSocket::ProcessToAccept - 클라이언트의 접속 처리 (FD_ACCEPT 처리)\n");
	
	int i;                // 최대 접속자 수를 카운팅할 때 사용
	sockaddr_in cli_addr; // 접속한 클라이언트에 대한 네트워크 정보를 저장할 변수
	int temp_client_info_size = sizeof(cli_addr);


	// 새로 접속을 시도하는 클라이언트와 통신할 소켓을 생성 (accept : 클라이언트 연결 수립, 소켓의 복사본 반환)
	// listen_socket의 복제를 만들어서 반환 -> 반환된 소켓은 클라이언트와 실제로 통신할 소켓 (listen_socket은 클라이언트의 접속만 받음)
	// h_client_socket : 클라이언트와 실제로 통신할 소켓, 접속한 클라이언트에 대한 네트워크 정보가 cli_addr에 저장된다
	SOCKET h_client_socket = accept((SOCKET)wParam, (sockaddr*)&cli_addr, &temp_client_info_size); 
	
	if (h_client_socket == INVALID_SOCKET)
	{
		TR("ServerSocket::ProcessToAccept - socket 생성 실패 (accept 실패)\n");
		return -1; // 소켓 생성이 실패한 경우!!
	}
	else // 소켓 생성 성공
	{
		UserData* p_user;            // 접속한 클라이언트에 대한 정보를 저장할 객체 (UserData 객체 or UserAccount 객체)
		wchar_t temp_ip_address[16]; // 접속한 클라이언트의 ip를 저장할 변수

		// 새로 접속한 클라이언트의 IP를 유니코드 형식의 문자열로 변환
		AsciiToUnicode(temp_ip_address, inet_ntoa(cli_addr.sin_addr));
		
		// 사용자 정보를 저장할 객체들 중에 아직 소켓을 배정받지 않은 객체를 찾아서 현재 접속한 사용자 정보를 저장
		for (i = 0; i < m_max_user_count; i++)
		{
			p_user = mp_user_list[i];

			// 사용자 정보의 소켓 핸들 값이 INVALID_SOCKET이면 해당 객체는 미사용중인 객체이다
			if (p_user->GetHandle() == INVALID_SOCKET)
			{	
				p_user->SetHandle(h_client_socket); // 사용자 정보에 소켓을 저장한다
				p_user->SetIP(temp_ip_address);     // 사용자 정보에 IP 주소를 저장한다

				// 연결된 클라이언트가 데이터를 전송(FD_READ)하거나 연결을 해제(FD_CLOSE)하면 
				// mh_notify_wnd 핸들 값에 해당하는 윈도우로 m_data_notify_id 메시지 ID가 전달되도록 비동기 설정
				WSAAsyncSelect(h_client_socket, mh_notify_wnd, m_data_notify_id, FD_READ | FD_CLOSE);

				// 새로운 사용자가 접속한 시점에 처리해야할 작업을 한다 (클래스 사용자가 직접 구현)
				AddWorkForAccept(p_user);
				break;
			}
		}

		// 최대 접속자수를 초과하여 더 이상 클라이언트의 접속을 허락할 수 없는 경우
		if (i == m_max_user_count)
		{
			TR("ServerSocket::ProcessToAccept - 최대 접속자수 초과\n");
			
			// 접속자수 초과에 대한 작업을 한다 (클래스 사용자가 직접 구현)
			ShowLimitError(temp_ip_address);

			closesocket(h_client_socket); // 접속한 소켓을 제거한다
			h_client_socket = INVALID_SOCKET;

			return -2; // 사용자 초과!!
		}

	} // 소켓 생성 성공

	TR("ServerSocket::ProcessToAccept - 클라이언트의 접속을 정상적으로 처리함\n");
	return 1; // 정상적으로 접속을 처리함!!
}


// 클라이언트의 네트워크 이벤트 처리 (FD_READ, FD_CLOSE 처리 함수)
// wParam: 메시지가 발생하게된 소켓의 핸들 (mh_listen_socket)
// lParam: 소켓에 에러가 있는지(WSAGETSELECTERROR) or 어떤 이벤트 때문에 발생했는지(WSAGETSELECTEVENT)
void ServerSocket::ProcessClientEvent(WPARAM wParam, LPARAM lParam)
{	
	TR("ServerSocket::ProcessClientEvent - 클라이언트의 네트워크 이벤트 처리 (FD_READ, FD_CLOSE 처리)\n");
	
	// 데이터 수신(FD_READ) 시에는 정해진 프로토콜 규약대로 읽어서 분석 후 처리하는 Socket::ProcessRecvEvent 함수 호출
	// 접속을 해제할 때 wParam에 저장된 소켓 핸들 값을 사용하여 어떤 사용자인지 찾는다
	// 그리고 접속을 해제할 때 필요한 작업을 하기 위해 AddWorkForCloseUser 함수 호출

	
	if (WSAGETSELECTEVENT(lParam) == FD_READ) // 클라이언트가 데이터를 전송한 경우 (데이터 수신)
	{
		Socket::ProcessRecvEvent((SOCKET)wParam); // 전송된 데이터를 분석해서 처리하는 함수 호출
	}
	else // FD_CLOSE (클라이언트가 접속을 해제한 경우)
	{
		UserData* p_data = FindUserData((SOCKET)wParam);  // 소켓 핸들을 사용하여 접속을 해제하려는 사용자를 찾는다
		AddWorkForCloseUser(p_data, 0);                   // 클라이언트 접속 해제시 추가로 작업할 내용을 수행 (상속받은 클래스에서 재정의)
		p_data->UserData::CloseSocket(0);                 // 접속을 해제한 소켓을 닫고 초기화
	}
}


// 클라이언트와 접속을 강제로 해제하기 (연결된 소켓을 닫고 초기화) (UserData::CloseSocket 사용)
// 접속 해제를 즉시 수행하기 위해, 내부적으로 링거옵션을 설정하여 데이터가 수신되는 중이라도 기다리지 않고 소켓을 제거
// 정상적인 프로토콜을 사용하는 클라이언트이지만 접속을 강제로 종료시켜야 하는 경우 (ex. 로그인 암호를 계속 틀리면)
// a_error_code -> 0: 정상종료, -1: 키값이 유효하지 않아서 종료, -2: 바디정보 수신중에 오류 발생
void ServerSocket::DisconnectSocket(SOCKET ah_socket, int a_error_code)
{
	TR("ServerSocket::DisconnectSocket - 클라이언트 접속 강제 해제\n");
	
	UserData* p_user_data = FindUserData(ah_socket); // 소켓 핸들을 사용하여 사용자 정보를 찾는다

	AddWorkForCloseUser(p_user_data, a_error_code);  // 접속을 해제하기 전에 작업해야 할 내용 처리 (상속받은 클래스에서 재정의)

	p_user_data->CloseSocket(1);                     // 해당 사용자의 소켓을 닫는다 (즉시)
}


// 수신된 데이터를 처리하는 함수 (정상적으로 'Head'와 'Body'를 수신한 경우 이 정보들을 사용하여 사용자가 원하는 작업을 처리)
// 접속된 클라이언트에서 데이터가 전송되면 FD_READ 이벤트 발생
// ServerSocket::ProcessClientEvent 호출 -> Socket::ProcessRecvEvent 호출 -> ServerSocket::ProcessRecvData 호출
// return -> 성공:1
int ServerSocket::ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size)
{
	TR("ServerSocket::ProcessRecvData - 수신된 데이터를 처리하는 함수\n");
	
	UserData* p_user_data = FindUserData(ah_socket); // 소켓 핸들값을 사용하여 이 소켓을 사용하는 사용자를 찾는다

	if (a_msg_id == 251) // 예약 메시지 251 : 클라이언트에게 대용량의 데이터를 전송하기 위해 사용 (message_id)
	{	
		char* p_send_data;													 // 데이터 전송을 위해 사용할 메모리
		BS send_size = p_user_data->GetSendMan()->GetPosition(&p_send_data); // 현재 전송 위치를 얻는다

		
		// 전송할 데이터가 더 있다면 예약 메시지 252를 사용하여 클라이언트에게 데이터를 전송
		if (p_user_data->GetSendMan()->IsProcessing()) // IsProcessing : 전송중이면 1반환, 전송 완료하면 0반환
		{
			Socket::SendFrameData(ah_socket, 252, p_send_data, send_size);
		}
		else
		{
			// 분할된 데이터의 마지막 부분이라면 (더 이상 전송할 데이터가 없으면) 예약 메시지 253을 사용하여 클라이언트에게 데이터를 전송
			Socket::SendFrameData(ah_socket, 253, p_send_data, send_size);

			p_user_data->GetSendMan()->DeleteData(); // 마지막 데이터를 전송하고 전송에 사용했던 메모리 삭제
			
			// 서버 소켓을 사용하는 윈도우에 전송이 완료되었음을 알려준다
			// 전송이 완료되었을 때 프로그램에 어떤 표시를 하고 싶다면 해당 윈도우에서 LM_SEND_COMPLETED 메시지를 체크하면 된다
			// 윈도우에서 전송이 완료되었음을 알리고 싶다면 LM_SEND_COMPLETED 메시지를 사용
			::PostMessage(mh_notify_wnd, LM_SEND_COMPLETED, (WPARAM)p_user_data, 0);
		}
	}
	else if (a_msg_id == 252) // 252: 대용량의 데이터를 수신할 때 사용하는 예약번호 (아직 추가로 수신할 데이터가 있다)
	{	
		// 수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합친다 (나누어서 보낸 데이터를 하나로 합치기)
		p_user_data->GetRecvMan()->AddData(ap_recv_data, a_body_size);

		// 252번은 아직 추가로 수신할 데이터가 있다는 뜻이기 때문에 예약 메시지 251번을 클라이언트에 전송하여 추가 데이터를 요청
		Socket::SendFrameData(ah_socket, 251, NULL, 0);
	}
	else if (a_msg_id == 253) // 253: 대용량의 데이터를 수신할 때 사용하는 예약번호 (더이상 전송할 데이터가 없다)
	{	
		// 수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합친다 (나누어서 보낸 데이터를 하나로 합치기)
		p_user_data->GetRecvMan()->AddData(ap_recv_data, a_body_size);

		// 253번은 데이터 수신이 완료되었다는 메시지이기 때문에 서버 소켓을 사용하는 윈도우에 완료되었음을 알린다
		// 윈도우에서 수신이 완료된 데이터를 사용하려면 LM_RECV_COMPLETED 메시지를 사용하면 된다
		// LM_RECV_COMPLETED 메시지를 수신한 처리기에서 수신할 때 사용한 메모리를 DeleteData 함수를 호출해세 제거해야 한다
		::PostMessage(mh_notify_wnd, LM_RECV_COMPLETED, (WPARAM)p_user_data, 0);
	}
	
	TR("ServerSocket::ProcessRecvData - 수신된 데이터를 정상적으로 처리함\n");

	// 수신된 데이터를 정상적으로 처리함. 만약, 수신 데이터를 처리하던 중에 소켓을 제거했으면 0을 반환해야 한다
	// 0을 반환하면 이 소켓에 대해 비동기 작업이 중단된다
	return 1;
	

	// 이 함수에서는 대용량 데이터의 전송 또는 수신에 대한 예약 메시지만 처리했기 때문에
	// 이 소켓의 사용자가 이 함수를 재정의 하여 자신만의 작업을 추가해야 한다
	// 이때 반드시 부모 클래스의 ProcessRecvData 함수를 호출하도록 구성해야 한다
	/*
	int MyServer::ProcessRecvData(...)
	{
		ServerSocket::ProcessRecvData(...);
		
		// ... 자신만의 작업 ... 
		return 1;
	}
	*/
}





// ClientSocket 클래스 메서드들

// a_connect_notify_id : FD_CONNECT 발생시 윈도우에 전달할 메시지 ID
// a_data_notify_id    : FD_READ, FD_CLOSE 발생시 윈도우에 전달할 메시지 ID
ClientSocket::ClientSocket(unsigned char a_valid_key, int a_connect_notify_id, int a_data_notify_id)
	:Socket(a_valid_key, a_data_notify_id)
{
	m_connect_flag = 0; // 접속 상태를 '접속 안됨'으로 초기화 한다
	mh_socket = INVALID_SOCKET; // 소켓 핸들을 초기화 한다
	m_connect_notify_id = a_connect_notify_id; // FD_CONNECT 이벤트 발생시에 사용할 윈도우 메시지 번호 초기화
}

ClientSocket::~ClientSocket()
{
	if (mh_socket != INVALID_SOCKET) // 서버와 통신하기 위한 소켓이 생성되어 있다면 소켓을 제거한다
	{
		closesocket(mh_socket);
		mh_socket = INVALID_SOCKET;
	}
		
}


// 서버에 접속하기 (return -> 중복 시도 또는 중복 접속:0, 성공:1)
// 서버에 접속하기 위해서는 '서버의IP주소'와 '포트번호'가 필요하고 
// '접속 결과를 윈도우 메시지로 전달 받기 위한 윈도우 핸들' 도 같이 매개변수로 넘겨받아야 한다 
int ClientSocket::ConnectToServer(const wchar_t* ap_ip_address, int a_port_num, HWND ah_notify_wnd)
{
	TR("ClientSocket::ConnectToServer - 서버에 접속하기\n");
	
	// 접속을 시도중이거나 접속된 상태라면 접속을 시도하지 않는다
	if (m_connect_flag != 0)
	{
		TR("ClientSocket::ConnectToServer - 중복 시도 또는 중복 접속 오류\n");
		return 0; // 중복 시도 또는 중복 접속 오류!!
	}

	mh_notify_wnd = ah_notify_wnd;               // 소켓 이벤트로 인한 윈도우 메시지를 받을 윈도우의 핸들을 저장한다
	mh_socket = socket(AF_INET, SOCK_STREAM, 0); // 서버와 통신할 소켓 생성 (TCP)

	char temp_ip_address[16];
	UnicodeToAscii(temp_ip_address, (wchar_t*)ap_ip_address); // 유니코드 형식으로 전달된 IP주소를 ASCII형식으로 변경

	// 서버에 접속하기 위해 서버의 IP주소와 포트번호로 접속 정보를 구성한다
	sockaddr_in srv_addr;
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr(temp_ip_address);
	srv_addr.sin_port = htons(a_port_num);

	// 서버에 접속하는 connect 함수가 응답없음 상태에 빠질수 있기 때문에 비동기를 설정
	// 서버 접속에 대한 결과(성공or실패)인 FD_CONNECT 이벤트가 발생하면 ah_notify_wnd에 해당하는 윈도우로
	// m_connect_notify_id 에 해당하는 윈도우 메시지를 전송한다
	WSAAsyncSelect(mh_socket, ah_notify_wnd, m_connect_notify_id, FD_CONNECT);

	// 서버에 접속을 시도한다 (접속 시도만 하고 함수를 빠져나온다 → 비동기 설정을 했기 때문)
	connect(mh_socket, (sockaddr*)&srv_addr, sizeof(srv_addr));

	m_connect_flag = 1; // 접속 상태를 '접속 시도중'으로 변경한다

	return 1;
}


// 접속 시도에 대한 결과 처리하기 (FD_CONNECT) (return -> 접속 성공:1, 접속 실패:0)
// connect 함수로 서버에 접속을 시도하면 그 결과가 윈도우 메시지로 전달되는데 그 메시지 처리기 에서 사용할 함수
// 클라이언트용 소켓에서는 소켓 핸들을 한 개만 사용하기 때문에 wParam에 전달된 소켓이 mh_socket 에 저장된 핸들 값과 동일해서 lParam만 이 함수로 전달
// 서버에 접속을 성공하면 서버와 통신하기 위해 FD_READ와 FD_CLOSE이벤트를 사용할 수 있도록 비동기 설정
// 서버에 접속을 실패하면 생성한 소켓을 제거하고 해당 변수 초기화
int ClientSocket::ResultOfConnection(LPARAM lParam)
{
	TR("ClientSocket::ResultOfConnection - 접속 시도에 대한 결과 처리 (FD_CONNECT)\n");
	
	// lParam: 소켓에 에러가 있는지(WSAGETSELECTERROR) or 어떤 이벤트 때문에 발생했는지(WSAGETSELECTEVENT)

	if (WSAGETSELECTERROR(lParam) == 0) // 접속 에러가 없음 (서버에 정공적으로 접속)
	{
		m_connect_flag = 2; // 접속 상태를 '접속중'으로 변경

		// 접속된 소켓으로 서버에서 데이터가 수신되거나 연결이 해제되었을 때 윈도우 메시지를 받을수 있도록 비동기 설정
		WSAAsyncSelect(mh_socket, mh_notify_wnd, m_data_notify_id, FD_READ | FD_CLOSE);

		TR("ClientSocket::ResultOfConnection - 서버 접속 성공\n");
		return 1; // 접속 성공
	}
	else // 접속에 실패함
	{
		closesocket(mh_socket);     // 서버와 통신하기 위해 만든 소켓을 제거
		mh_socket = INVALID_SOCKET; // 소켓을 초기화 
		m_connect_flag = 0;         // 접속 상태를 '접속 해제'로 변경
		
		TR("ClientSocket::ResultOfConnection - 서버 접속 실패\n");
		return 0; // 접속 실패!!
	}

}


// 데이터 수신 처리와 서버 연결 해제에 대한 처리 (FD_READ, FD_CLOSE) (return -> 발생한 이벤트 종류 FD_READ:1, FD_CLOSE:0)
// 데이터가 수신되어 FD_READ 이벤트가 발생했을 때는 Socket::ProcessRecvEvent 함수를 호출하여 
// 전송된 데이터를 수신하고 약속된 프로토콜 대로 분석하여 내부적으로 ProcessRecvData 함수를 호출
// 접속이 해제되어 FD_CLOSE 이벤트가 발생하면 소켓을 제거하고 핸들을 저장했던 변수를 초기화
int ClientSocket::ProcessServerEvent(WPARAM wParam, LPARAM lParam)
{
	TR("ClientSocket::ProcessServerEvent - 데이터 수신 처리와 서버 연결 해제에 대한 처리 (FD_READ, FD_CLOSE)\n");
	
	// 접속이 해제 되었을 때, 추가적인 메시지를 사용하지 않고 이 함수의 반환값으로 구별해서 사용할 수 있도록
	// FD_READ는 1, FD_CLOSE는 0값을 반환하도록 구현
	int state;
	
	if (WSAGETSELECTEVENT(lParam) == FD_READ) // 서버에서 데이터를 전송한 경우
	{
		// 수신된 데이터를 처리하기 위한 함수 호출 (ProcessRecvEvent: 에러가 발생하면 소켓을 제거하고 0반환, 정상적으로 처리했으면 1반환)
		if (Socket::ProcessRecvEvent((SOCKET)wParam) == 0)
		{
			// FD_READ 메시지가 발생했더라도 ProcessRecvEvent에서 0을 반환하면 데이터 수신중 에러가 발생하여 소켓을 제거한 것이므로 FD_CLOSE 메시지를 보내줘야한다
			state = 0;
		}
		else state = 1;
	}
	else // 서버가 접속을 해제한 경우 (FD_CLOSE)
	{
		state = 0;
		m_connect_flag = 0;         // 접속 상태를 '접속 해제'로 변경
		closesocket(mh_socket);     // 서버와 통신하기 위해 만든 소켓을 제거
		mh_socket = INVALID_SOCKET; // 소켓을 초기화 
	}

	return state; // 이벤트 종류를 반환 (FD_READ | FD_CLOSE)
}


// 데이터 전송 함수 (전달된 정보를 가지고 mp_send_data 메모리에 약속된 Head 정보를 구성해서 전송) (return -> 성공:1, 실패:0)
// 클라이언트 소켓 클래스는 하나의 소켓만 사용하기 때문에 서버로 데이터를 전송할 때마다 소켓 핸들을 GetHandle 함수로 얻어와서 사용하기 불편하다
// 핸들을 적지 않고 SendFrameData 함수를 사용할 수 있도록 Socket::SendFrameData 함수를 오버로딩
// 내부적으로 mh_socket을 사용하여 Socket 클래스의 SendFrameData 함수를 다시 호출
int ClientSocket::SendFrameData(unsigned char a_message_id, const char* ap_body_data, BS a_body_size)
{
	TR("ClientSocket::SendFrameData - 데이터 전송\n");
	
	return Socket::SendFrameData(mh_socket, a_message_id, ap_body_data, a_body_size);
}


// 서버와 접속을 강제로 해제하기
// 서버와 접속해서 통신중에 사용자가 접속해제 버튼을 누르면 DisconnectSocket 함수를 사용하여 강제로 소켓을 제거
// 서버와의 접속 해제를 즉시 수행하기 위해, 내부적으로 링거옵션을 설정하여 서버로부터 데이터가 수신되는 중이라도 기다리지 않고 소켓을 제거
void ClientSocket::DisconnectSocket(SOCKET ah_socket, int a_error_code)
{
	TR("ClientSocket::DisconnectSocket - 서버와 접속 해제\n");

	m_connect_flag = 0; // 접속 상태를 '접속 해제'로 변경

	LINGER temp_linger = { TRUE, 0 };                                                       // 데이터가 송수신되는 것과 상관없이 소켓을 바로 닫겠다
	setsockopt(mh_socket, SOL_SOCKET, SO_LINGER, (char*)&temp_linger, sizeof(temp_linger)); // mh_socket소켓의 LINGER 옵션 변경

	closesocket(mh_socket);     // 소켓 제거
	mh_socket = INVALID_SOCKET; // 소캣 핸들 값을 저장하는 변수 초기화
}


// 수신된 데이터를 처리하는 함수
// 접속된 서버에서 데이터가 전송되면 FD_READ 이벤트가 발생하여 ProcessServerEvent 함수가 호출되고 ProcessRecvData 함수가 호출된다
int ClientSocket::ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size)
{
	TR("ClientSocket::ProcessRecvData - 수신된 데이터 처리\n");
	
	if (a_msg_id == 251) // 예약 메시지 251 : 서버에 큰용량의 데이터를 전송하기 위해 사용 (message_id)
	{
		char* p_send_data;                                   // 데이터 전송을 위해 사용할 메모리
		BS send_size = m_send_man.GetPosition(&p_send_data); // 현재 전송 위치를 얻는다

		// 전송할 데이터가 더 있다면 예약 메시지 번호인 252를 사용하여 서버에게 데이터를 전송한다
		if (m_send_man.IsProcessing()) // IsProcessing : 전송중이면 1반환, 전송 완료하면 0반환
		{
			Socket::SendFrameData(mh_socket, 252, p_send_data, send_size);
		}
		else
		{
			// 지금이 분할된 데이터의 마지막 부분이라면(더이상 전송할 데이터가 없으면) 예약 메시지 번호인 253번을 사용하여 서버에게 데이터를 전송한다
			Socket::SendFrameData(mh_socket, 253, p_send_data, send_size);

			m_send_man.DeleteData(); // 마지막 데이터를 전송하고 전송에 사용했던 메모리 삭제

			// 클라이언트 소켓을 사용하는 윈도우에 전송이 완료되었음을 알려준다
			// 전송이 완료되었을 때 프로그램에 어떤 표시를 하고 싶다면 해당 윈도우에서 LM_SEND_COMPLETED 메시지를 체크하면 된다
			::PostMessage(mh_notify_wnd, LM_SEND_COMPLETED, 0, 0);
		}
	}
	else if (a_msg_id == 252) // 252번은 대용량의 데이터를 수신할 때 사용하는 예약번호 (아직 추가로 수신할 데이터가 있다)
	{
		// 수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합친다 (나누어서 보낸 데이터를 하나로 합치기)
		m_recv_man.AddData(ap_recv_data, a_body_size);

		// 252번은 아직 추가로 수신할 데이터가 있다는 뜻이기 때문에 예약 메시지 251번을 서버에 전송하여 추가 데이터를 요청한다
		SendFrameData(251, NULL, 0);
	}
	else if (a_msg_id == 253) // 253번은 대용량의 데이터를 수신할때 사용하는 예약된 메시지 번호 (지금이 분할된 마지막 부분이라면(더이상 전송할 데이터가 없으면))
	{
		
		// 수신된 데이터는 수신을 관리하는 객체로 넘겨서 데이터를 합친다
		m_recv_man.AddData(ap_recv_data, a_body_size);

		// 253번은 데이터 수신이 완료되었다는 메시지이기 때문에 클라이언트 소켓을 사용하는 윈도우에 완료되었음을 알려준다
		// 윈도우에서 수신이 완료된 데이터를 사용하려면 LM_RECV_COMPLETED 메시지를 사용하면 된다
		// LM_RECV_COMPLETED 메시지를 수신한 처리기에서 수신할 때 사용한 메모리를 DeleteData 함수를 호출해세 제거해야 한다
		::PostMessage(mh_notify_wnd, LM_RECV_COMPLETED, 0, 0);
	}

	// 수신된 데이터를 정상적으로 처리함. 만약, 수신 데이터를 처리하던 중에 소켓을 제거했으면 0을 반환해야 한다
	// 0을 반환하면 이 소켓에 대해 비동기 작업이 중단된다
	return 1;

	// 이 함수에서는 대용량 데이터의 전송 또는 수신에 대한 예약 메시지만 처리했기 때문에
	// 이 소켓의 사용자가 이 함수를 재정의하여 자신만의 작업을 추가해야 한다
	// 이때 반드시 부모 클래스의 ProcessRecvData 함수를 호출하도록 구성해야 한다
}
