#pragma once

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
	
	// 총 전송된 크기와 할당된 메모리 전체 크기를 비교하여 전송이 완료되었는지 체크 (전송 완료: 1반환) 
	inline int IsProcessing() { return m_total_size == m_current_size; }
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
#define LM_SEND_COMPLETED 29001
#define LM_RECV_COMPLETED 29002



// 서버, 클라이언트 클래스의 중복 기능을 구현하는 클래스
// 전송할 데이터를 나누거나 수신된 데이터를 하나로 합치는 작업
class Socket
{
protected:
	unsigned char m_valid_key; // 구분값 (프로토콜의 유효성을 체크하기 위한 값)
	char* mp_send_data, * mp_recv_data; // 전송, 수신에 사용할 메모리
	HWND mh_notify_wnd; // 윈도우 핸들 (소켓에 새로운 데이터가 수신되었거나 연결 해제되었을 때 발생하는 메시지를 수신할 윈도우 핸들)
	int m_data_notify_id; // 데이터가 수신되거나 상대편이 접속을 해제했을 때 사용할 메시지 ID. 소켓에 비동기 이벤트(FD_READ | FD_CLOSE)가 발생했을 때 윈도우 핸들에 넘겨줄 메시지 ID

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



// 