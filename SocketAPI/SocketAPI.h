#pragma once

#include "pch.h"
#include "framework.h"
#include <WinSock2.h>

// 소켓으로 데이터를 주고받을때 큰용량의 데이터를 한 번에 전송하면 전송 부하만 심해질 뿐 전송이 제대로 되지 않기 때문에
// 데이터를 나누어서 전송하고 수신하는 측에서 데이터를 다시 합쳐서 사용한다 (Socket class)


// 전송, 수신 공통 사용 클래스
// 전송할 데이터나 수신될 데이터에 대한 메모리 할당, 삭제
class ExchangeManager 
{
protected:
	int m_total_size, m_current_size; // 전송 또는 수신을 위해 할당된 메모리의 전체 크기와 현재 작업중인 크기
	char* mp_data; // 전송 또는 수신을 위해 할당된 메모리의 시작 주소

public:
	ExchangeManager(); // 멤버 변수 초기화
	~ExchangeManager(); // 전송, 수신을 위해 할당된 메모리 제거

	char* MemoryAlloc(int a_data_size); // 전송, 수신에 사용할 메모리 할당 (a_data_size에 할당할 크기를 명시하면 할당된 메모리 주소 반환)
	void DeleteData(); // 전송, 수신에 사용된 메모리 제거

	inline int GetTotalSize() { return m_total_size; } // 할당된 메모리 크기 반환
	inline int GetCurrentSize() { return m_current_size; } // 현재 작업중인 메모리 위치 반환
};


// SendManager, RecvManager : 서버, 클라이언트 소켓 클래스에서 큰 크기의 데이터를 전송 또는 수신할 때 사용


// 전송 측 클래스
// 큰 데이터를 전송할 때는 일정한 크기로 데이터를 나누어서 전송해야 한다.
// 하나의 메모리로 구성해서 일정한 단위로 전송 시작 위치만 변경하고 그 위치에 자신이 정한 크기만큼 전송
class SendManager : public ExchangeManager
{
public:

	// 전송할 위치와 크기 계산 (전송 시작 위치 변경) (큰 크기의 데이터를 나눌때 사용)
	int GetPosition(char** ap_data, int a_data_size = 2048); 
	
	// 총 전송된 크기와 할당된 메모리 전체 크기를 비교하여 전송이 완료되었는지 체크
	inline int IsProcessing() { return m_total_size != m_current_size; } // 전송할 데이터가 남아있으면 1반환 (전송중)
};



// 수신 측 클래스
// 수신될 데이터의 총 크기로 메모리를 만들어 놓고 
// 나누어져서 전송되는 데이터를 하나의 메모리에 합치는 작업 (저장할 위치를 옮겨가면서 수신된 데이터를 저장)
// 전송이 완료된 후, 하나로 합쳐진 데이터의 시작 주소를 얻는 함수
class RecvManager : public ExchangeManager
{
public:
	int AddData(char* ap_data, int a_size); // 수신된 데이터를 기존에 수신된 데이터에 추가 

	inline char* GetData() { return mp_data; } // 수신된 데이터를 하나로 합친 메모리의 시작 주소를 얻는다
};



// 네트워크 프레임 구조 (Head + Body)
// Head 4byte : 구분값 1byte, Message ID 1byte, Body size 2byte
// 구분값 : 네트워크로 데이터가 수신되었을 때, 이 데이터가 내가 원하는 데이터인지 체크 (1byte)
// Message ID : Body에 저장된 데이터의 종류를 구분 (ex. Message ID가 1이면 Body에 저장된 값이 로그인 정보, 2이면 채팅 데이터) (1byte)
// Body size : Body에 저장된 데이터의 크기 (2byte)
// Body : 프로그램이 어떤 작업을 위해 실제로 사용할 정보가 저장 (크기 : Head의 Body size)

typedef unsigned short BS; // Body size (2byte)
#define HEAD_SIZE 2+sizeof(BS) // Head size (key + message_id + body_size)
#define LM_SEND_COMPLETED 29001 // 대용량 데이터가 전송 완료 되었을 때 사용할 메시지 (윈도우에서 전송이 완료되었음을 알리고 싶다면 LM_SEND_COMPLETED 메시지를 사용)
#define LM_RECV_COMPLETED 29002 // 대용량 데이터가 전송 완료 되었을 때 사용할 메시지 (윈도우에서 수신이 완료된 데이터를 사용하려면 LM_RECV_COMPLETED 메시지를 사용)
// a_accept_notify_id = 25001 // FD_ACCEPT 발생시 윈도우에 전달할 메시지 ID
// a_data_notify_id = 25002   // FD_READ, FD_CLOSE 발생시 윈도우에 전달할 메시지 ID



// 서버, 클라이언트 클래스의 중복 기능을 구현하는 클래스
// 전송할 데이터를 나누거나 수신된 데이터를 하나로 합치는 작업
class Socket
{
protected:
	unsigned char m_valid_key; // 구분값 (프로토콜의 유효성을 체크하기 위한 값)
	char* mp_send_data, * mp_recv_data; // 전송, 수신에 사용할 메모리
	HWND mh_notify_wnd; // 윈도우 핸들 (소켓에 새로운 데이터가 수신되었거나 연결 해제되었을 때 발생하는 메시지를 수신할 윈도우 핸들)
	int m_data_notify_id; // 데이터가 수신되거나 상대편이 접속을 해제했을 때 사용할 메시지 ID (FD_READ, FD_CLOSE 이벤트시에 발생할 메시지) (윈도우 핸들에 넘겨줄 메시지 ID)


public:
	Socket(unsigned char a_valid_key, int a_data_notify_id); // 객체 생성시에 프로토콜 구분 값과 데이터 수신 및 연결 해제에 사용할 메시지 ID 지정
	~Socket();


	// 데이터 전송 함수 (전달된 정보를 가지고 mp_send_data 메모리에 약속된 Head 정보를 구성해서 전송)
	int SendFrameData(SOCKET ah_socket, unsigned char a_message_id, const char* ap_body_data, BS a_body_size);

	// 안정적인 데이터 수신 (재시도 수신)
	int ReceiveData(SOCKET ah_socket, BS a_body_size);

	// 데이터가 수신되었을 때 수신된 데이터를 처리하는 함수
	void ProcessRecvEvent(SOCKET ah_socket);


	// DisconnectSocket, ProcessRecvData 함수는 서버 소켓인지 클라이언트 소켓인지에 따라 내용이 달라질 수 있으므로 
	// 가상함수로 선언하고 사용하는 클래스에서 재정의하여 사용
	
	// 접속된 대상을 끊는 함수 (자식 클래스에서 재정의)	
	virtual void DisconnectSocket(SOCKET ah_socket, int a_error_code) = 0; 
	// 수신된 데이터를 처리하는 함수 (자식 클래스에서 재정의)
	virtual int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size) = 0; 


	// IP 주소의 문자열 형식 변환 함수 (ASCII 1byte, 유니코드 2byte)
	static void AsciiToUnicode(wchar_t* ap_dest_ip, char* ap_src_ip); // ASCII 형식의 문자열을 유니코드로 변환
	static void UnicodeToAscii(char* ap_dest_ip, wchar_t* ap_src_ip); // 유니코드 형식의 문자열을 ASCII로 변환
};



// 사용자 정보 관리용 클래스 (하나의 사용자 정보를 저장하기 위한 클래스)
// 서버용 소켓에서 접속된 클라이언트를 관리하기 위해 사용
class UserData
{
protected:
	// 클라이언트와 통신하기 위해 사용할 소켓 핸들 
	SOCKET mh_socket; // 클라이언트 소켓이 접속을 시도하면 서버는 accept함수를 통해 해당 클라이언트와 통신할 새로운 소켓 핸들을 만든다

	// 클라이언트에게 큰 데이터를 전송하기 위해 사용할 객체
	SendManager* mp_send_man; // mh_socket과 연결되어 있는 클라이언트 소켓에 큰 크기의 데이터를 전송할 때 사용 

	// 클라이언트에게서 큰 데이터를 수신하기 위해 사용할 객체
	RecvManager* mp_recv_man; // mh_socket과 연결되어 있는 클라이언트 소켓으로부터 큰 크기의 데이터를 수신할 때 사용 

	wchar_t m_ip_address[16]; // 접속한 클라이언트의 IP 주소

public:
	UserData(); // 멤버변수 초기화, 전송과 수신에 사용할 객체 생성
	~UserData(); // 사용하던 소켓 제거, 전송과 수신에 사용한 객체 제거


	// 멤버변수의 값을 클래스 외부에서 사용할 수 있도록 해줄 인터페이스 함수들
	inline SOCKET GetHandle() { return mh_socket; }
	inline void SetHandle(SOCKET ah_socket) { mh_socket = ah_socket; }
	inline SendManager* GetSendMan() { return mp_send_man; }
	inline RecvManager* GetRecvMan() { return mp_recv_man; }
	inline wchar_t* GetIP() { return m_ip_address; }
	inline void SetIP(const wchar_t* ap_ip_address) { wcscpy(m_ip_address, ap_ip_address);  }


	// 서버 소켓 클래스에서 클라이언트가 접속을 해제할 때 소켓을 닫고 초기화하는 작업을 해야 하는데 클라이언트의 소켓 핸들을
	// 이 클래스가 가지고 있어서 매번 GetHandle, SetHandle 함수를 반복적으로 사용해야 하는 불편함이 있다
	// 그래서 아래와 같이 CloseSocket 함수를 추가로 제공한다
	void CloseSocket(int a_linger_flag); // 연결된 소켓을 닫고 초기화


	// 다형성 적용 시, 동일한 클래스를 확장할 때 이 함수를 사용한다 (UserAccount)
	virtual UserData* CreateObject() { return new UserData; }
};



// 로그인 개념을 사용하기 위해 아이디나 암호 같은 정보를 추가로 관리할 필요가 있다면
// UserData에서 상속받은 새로운 클래스 UserAccount를 만들어서 사용
// UserAccount 클래스는 UserData에서 상속받았기 때문에 다형성을 적용하면 UserAccount로 만들어진 객체의 주소를 UserData클래스의 포인터로 사용할 수 있다
// 따라서 서버용 소켓이 UserData클래스의 포인터를 사용해서 프로그램이 되어 있더라도 
// UserAccount로 메모리를 동적할당해서 넘겨주면 서버용 소켓 내부적으로는 UserAccount클래스로 만들어진 객체가 사용된다
class UserAccount : public UserData
{
protected:
	wchar_t id[32]; // 사용자의 아이디
	wchar_t password[32]; // 사용자의 비밀번호

public:
	wchar_t* GetID() { return id; }
	void SetID(const wchar_t* ap_id) { wcscpy(id, ap_id); }
	wchar_t* GetPassword() { return password; }
	void SetPassword(const wchar_t* ap_password) { wcscpy(password, ap_password); }


	// UserAccount 클래스는 서버용 소켓을 만들고 나서 나중에 사용자가 필요해서 정의하는 것이기 때문에 서버용 소켓을 만드는 시점에는 UserAccount 클래스가 없다
	// 이 부분을 해결하기 위해 CreateObject 함수 사용

	// 다형성 적용 시, 동일한 클래스를 확장할 때 이 함수를 사용한다
	virtual UserData* CreateObject() { return new UserAccount; }
	// CreateObject 함수가 추가되면 다형성이 적용된 소스에서 UserAccount클래스를 모르더라도 CreateObject함수를 사용하면 현재 사용하는 객체와 동일한 객체를 생성할 수 있다
	// 이것이 가능하려면 소켓 클래스 생성 시에 new UserAccount값을 인자로 넘겨서 사용자 정보를 저장할 클래스를 생성해서 넘겨줘야 된다

	// ex)
	// ServerSocket my_server(0x27, MAX_USER_COUNT, new UserAccount); // 사용자 정보를 UserAccount로 사용하고 싶다면
	// ServerSocket my_server(0x27, MAX_USER_COUNT, new UserData);    // 사용자 정보를 UserData로 사용하고 싶다면
};



// 접속된 사용자 관리 -> UserData 클래스는 하나의 사용자 정보를 저장하기 위한 클래스이기 때문에 
// 이 클래스를 사용하여 여러 명의 사용자를 관리할 수 있도록 구현해야 한다 -> 서버용 소켓 클래스에서 구현
// [서버 소켓 클래스에서 사용자 관리하는 부분을 별도의 클래스로 분리해서 코드로 구성해보기]



// 서버용 소켓 클래스
// 서버는 listen (클라이언트의 연결 요청 대기 ) 기능을 수행하는 작업을 제일 먼저 해야 한다.
// listen 작업이 시작되면 새로운 사용자들이 접속을 하게 되고 접속의 허락과 동시에 사용자 관리 개념이 필요하다.
// 접속한 사용자들과 정해진 프로토콜로 통신을 하면서 접속을 해제할 때 까지 네트워크 작업이 지속된다
class ServerSocket : public Socket
{
protected:
	SOCKET mh_listen_socket; // listen 작업에 사용할 소켓 핸들 (클라이언트의 접속을 받아주는 소켓)
	int m_accept_notify_id; // 새로운 클라이언트가 접속했을 때 발생할 메시지 ID 값 (FD_ACCEPT 이벤트시에 발생할 메시지 ID)
	unsigned short m_max_user_count; // 서버가 관리할 최대 사용자 수 (서버에 접속 가능한 최대 사용자 수 - 최대 65535명)
	UserData** mp_user_list; // 최대 사용자를 저장하기 위해서 사용할 객체들 (이중 포인터)

	// UserData** mp_user_list;  ->  객체 생성자를 직접 사용할 수 있는 형태로 만들기 위해 이중 포인터 사용
	// 객체 생성자에서 mp_user_list에 접속하는 최대 사용자 수만큼 메모리를 할당해야 하는데 아래와 같이 일차원 포인터를 사용해서 구성할 수도 있다
	// UserData* mp_user_list = new UserData[m_max_user_count];
	// 이렇게 구성하면 사용자 정보를 저장할 객체가 UserData로 고정되어 버리기 때문에 다형성 구조를 만드는데 문제가 생긴다

public:
	ServerSocket(unsigned char a_valid_key, unsigned short a_max_user_count, UserData* ap_user_data, int a_accept_notify_id = 25001, int a_data_notify_id = 25002);
	virtual ~ServerSocket(); // 가상 파괴자

	// 가상 파괴자가 필요한 경우 : 기초, 파생 클래스에서 파괴자를 정의한 경우
	// 기초 클래스의 파괴자가 가상이 아니라면 포인터형에 해당하는(기초 클래스) 파괴자만 호출된다 (객체형에 해당하는 파괴자가 호출되지 않는다)
	// B클래스가 A로부터 상속받고 A,B 클래스 모두 파괴자가 있는 상태에서 A클래스의 파괴자가 가상이 아니라면
	// A* test = new B(); delete test; -> A의 파괴자만 호출 (B의 파괴자는 호출되지 않는다)


	// 서버 서비스의 시작 (listen 작업을 수행할 함수)
	int StartServer(const wchar_t* ap_ip_address, int a_port, HWND ah_notify_wnd);

	// 클라이언트의 접속 처리 (FD_ACCEPT 처리 함수)
	int ProcessToAccept(WPARAM wParam, LPARAM lParam);


	// 추가작업
	virtual void AddWorkForAccept(UserData* ap_user) = 0; // Accept 시에 추가적으로 해야할 작업이 있다면 이 함수를 오버라이딩해서 처리	
	virtual void ShowLimitError(const wchar_t* ap_ip_address) = 0; // 최대 사용자수 초과시에 추가적으로 해야할 작업이 있다면 이 함수를 오버라이딩해서 처리


	// 클라이언트의 네트워크 이벤트 처리 (FD_READ, FD_CLOSE 처리 함수)
	void ProcessClientEvent(WPARAM wParam, LPARAM lParam);


	// 클라이언트 접속 해제시에 추가적인 작업이 필요하다면 상속받은 클래스에서 이 함수를 재정의해서 사용
	// a_error_code : 0이면 정상종료, -1이면 키값이 유효하지 않아서 종료, -2이면 바디정보 수신중에 오류 발생
	virtual void AddWorkForCloseUser(UserData* ap_user, int a_error_code) = 0;
	

	// 각종 네트워크 이벤트에 의해 메시지가 발생하면, 해당 이벤트가 발생한 소켓의 핸들이 메시지의 wParam항목으로 전달된다
	// 서버는 여러 명의 사용자가 동시에 접속해서 사용하기 때문에 해당 소켓이 어떤 사용자의 소켓인지 알 수 없다
	
	// 소켓 핸들을 사용하여 어떤 사용자인지 찾는다 (찾으면 사용자의 위치를 반환)
	inline int FindUserIndex(SOCKET ah_socket)
	{
		for (int i = 0; i < m_max_user_count; i++)
			if (mp_user_list[i]->GetHandle() == ah_socket) return i;

		return -1;
	}

	// 소켓 핸들을 사용하여 어떤 사용자인지 찾는다 (찾으면 사용자 정보를 관리하는 객체의 주소를 반환)
	inline UserData* FindUserData(SOCKET ah_socket)
	{
		for (int i = 0; i < m_max_user_count; i++)
			if (mp_user_list[i]->GetHandle() == ah_socket) return mp_user_list[i];

		return NULL;
	}


	// 클라이언트와 접속을 강제로 해제하기 (연결된 소켓을 닫고 초기화) (UserData::CloseSocket 사용)
	void DisconnectSocket(SOCKET ah_socket, int a_error_code);


	// 수신된 데이터를 처리하는 함수
	int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size);


	// 서버에서 관리하는 전체 사용자에 대한 정보나 최대 사용자 수를 외부에서 이용할 수 있도록 해주는 함수
	inline UserData** GetUserList() { return mp_user_list; } // 전체 사용자에 대한 정보
	unsigned short GetMaxUserCount() { return m_max_user_count; } // 최대 사용자 수
};



// 클라이언트용 소켓 클래스
// 서버에 접속해서 서비스를 제공받는 형태이기 때문에 소켓을 한개만 사용해도 충분히 작업이 가능
// 서버에 접속을 시도할 때 지연시간이 있을 수 있고 접속을 실패할 수도 있기 때문에 접속 상태를 별도로 가지고 있어야 한다
// connect 함수를 사용해서 서버에 접속을 시도하면 서버에 문제가 있을 경우 응답없음 상태에 빠질 수 있으므로 비동기 설정
// 서버 접속에 대한 결과 이벤트인 FD_CONNECT가 발생하면 클라이언트 소켓을 사용하는 윈도우에 메시지 전달
// 서버에 큰 용량의 데이터를 전송하거나 서버로부터 큰 용량의 데이터를 수신 받을때 사용할 객체를 가지고 있어야 한다
class ClientSocket : public Socket
{
protected:
	SOCKET mh_socket; // 서버와 통신하기 위해 사용할 소켓 핸들
	char m_connect_flag; // 0: 접속 안됨, 1: 접속 시도중, 2: 접속중
	int m_connect_notify_id; // 서버에 접속을 시도한 결과를 알려줄 윈도우 메시지 ID (FD_CONNECT)
	SendManager m_send_man; // 서버에 큰 데이터를 전송하기 위해 사용할 객체
	RecvManager m_recv_man; // 서버에 큰 데이터를 수신하기 위해 사용할 객체

public:
	ClientSocket(unsigned char a_valid_key, int a_connect_notify_id, int a_data_notify_id);
	~ClientSocket();


	// 서버에 접속하기
	int ConnectToServer(const wchar_t* ap_ip_address, int a_port_num, HWND ah_notify_wnd); 

	// 접속 시도에 대한 결과 처리하기 (FD_CONNECT)
	int ResultOfConnection(LPARAM lParam); 

	// 데이터 수신 처리와 서버 연결 해제에 대한 처리 (FD_READ, FD_CLOSE)
	int ProcessServerEvent(WPARAM wParam, LPARAM lParam); 

	// 서버와 접속을 강제로 해제하기
	void DisconnectSocket(SOCKET ah_socket, int a_error_code);

	
	// 데이터 전송 함수 (전달된 정보를 가지고 mp_send_data 메모리에 약속된 Head 정보를 구성해서 전송)
	int SendFrameData(unsigned char a_message_id, const char* ap_body_data, BS a_body_size); // Socket::SendFrameData 오버로딩

	// 수신된 데이터를 처리하는 함수
	int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size);


	 
	inline int IsConnected() { return m_connect_flag == 2; } // 서버와의 접속상태를 알고 싶을때 사용 (반환값 0:해제상태, 1: 접속상태)  내부적으로는 상태를 세가지로 관리하지만 외부에 알려줄때는 두가지 상태로 알려준다 ('접속 시도중' 상태는 해제로 간주한다)
	inline SOCKET GetHandle() { return mh_socket; } // 서버와 통신하기 위해 생성한 소켓의 핸들 값을 알고 싶을 때 사용
};