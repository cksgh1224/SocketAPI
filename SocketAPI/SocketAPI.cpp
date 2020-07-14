// SocketAPI.cpp : 정적 라이브러리를 위한 함수를 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include "SocketAPI.h"
#include <cstring>

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
	// 전송할 데이터를 하나의 메모리로 관리하고 메모리 시작 위치부터 2048byte (a_data_size) 단위로 데이터를 전송
	// 변경된 시작 위치는 ap_data에 넣는다, 실제 전송해야할 데이터 크기를 함수 반환값으로 알려준다
	// ex) 전송할 데이터의 전체 크기가 2050이면 GetPosition 함수를 처음 사용했을 때는 2048반환, 두 번째 호출때는 2반환
	
	// **ap_data : 전송할 메모리의 시작 주소를 받아야 하는데 주소를 받아오려면 포인터의 주소를 넘겨야 하기 때문에 2차원 포인터 사용
	*ap_data = mp_data + m_current_size; // 새로운 전송 위치에 대한 주소를 첫번째 인자에 저장 

	// 전송 크기를 계산하기 위해 2048byte(a_data_size)를 더한 크기가 최대 크기보다 작은지 체크
	if (m_current_size + a_data_size < m_total_size)
	{	
		// 최대 크기보다 작은 경우 2048byte 만큼 전송하면 된다
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
	// m_current_size: 일정 크기의 데이터가 수신될 때마다 저장할 위치를 계산하기 위해 현재 수신된 데이터의 총 크기를 기억할 변수
	
	memcpy(mp_data + m_current_size, ap_data, a_size); // 기존 수신 데이터(mp_data + m_current_size)위치에 수신된 데이터(ap_data) 를 복사 (a_size만큼)
	m_current_size += a_size; // 총 수신 크기 계산
	return m_current_size; // 현재 수신된 데이터의 크기를 반환
}





// Socket 클래스 메서드들

// 객체 생성시에 프로토콜 구분 값과 데이터 수신 및 연결 해제에 사용할 메시지 ID 지정
Socket::Socket(unsigned char a_valid_key, int a_data_notify_id)
{
	m_valid_key = a_valid_key; // 사용자가 지정한 프로토콜 구분 값 저장
	mp_send_data = new char[8192]; // 전송용으로 사용할 메모리 할당 (8kbyte)
	*(unsigned char*)mp_send_data = a_valid_key; // 전송할 메모리의 선두 1바이트에 구분값을 넣는다

	mp_recv_data = new char[8192]; // 수신용으로 사용할 메모리 할당 (8kbyte)
	
	mh_notify_wnd = nullptr; // 데이터 수신 및 연결 해제 메시지를 받을 윈도우 핸들 값 초기화
	m_data_notify_id = a_data_notify_id; // 데이터 수신 및 연결 해제 메시지 ID로 사용할 값 (FD_READ, FD_CLOSE)

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


// 데이터 전송 함수 (전달된 정보를 가지고 mp_send_data 메모리에 약속된 Head 정보를 구성해서 전송)
int Socket::SendFrameData(SOCKET ah_socket, unsigned char a_message_id, const char* ap_body_data, BS a_body_size)
{
	// 메시지 ID를 두 번째 바이트에 저장 (첫 번째 바이트에는 구분값이 이미 들어있다 - 객체 생성자)
	*(unsigned char*)(mp_send_data + 1) = a_message_id;

	// 세번째와 네번째 바이트에 'Body'데이터의 크기 저장 (현재 2byte 크기) 
	*(BS*)(mp_send_data + 2) = a_body_size;

	// 다섯번째 위치에 (현재 HEAD_SIZE는 4byte) 'Body'를 구성하는 데이터 복사 
	memcpy(mp_send_data + HEAD_SIZE, ap_body_data, a_body_size);

	// 소켓으로 데이터 전송. 전송 총량은 Head + Body
	// send() 함수의 반환 값 : 전송된 총 바이트수
	// (ah_socket) 소켓으로 (mp_send_data) 데이터를 (HEAD_SIZE + a_body_size) 크기만큼 보내겠다
	if (send(ah_socket, mp_send_data, HEAD_SIZE + a_body_size, 0) == HEAD_SIZE + a_body_size)
		return 1; // 전송 성공

	return 0; // 전송 실패	
}


// 안정적인 데이터 수신 (재시도 수신)
int Socket::ReceiveData(SOCKET ah_socket, BS a_body_size)
{
	// 수신되는 데이터가 크거나 네트워크 상태가 좋지 못하면 데이터가 여러 번에 걸쳐 나누어져 수신될 수 있다
	// 정확한 수신을 위해 재시도 읽기를 해야 함 (반복문)
	
	BS total_size = 0; // 총 읽은 데이터
	int retry = 0; // 재시도 횟수
	int read_size; // recv 함수의 반환값 저장 (recv: 실제로 읽은 데이터의 크기 값을 반환)

	// a_body_size에 지정한 크기만큼 한번에 수신하지 못 할 수 있으므로 반복문을 이용해 여러번 재시도하며 읽는다
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

			// 총 수신된 크기가 a_body_size보다 작으면 수신 상태가 좋지 않다는 뜻, 약간의 지연을 주면서 다시 읽기를 시도
			if (total_size < a_body_size) Sleep(5);

			retry = 0; // 재시도 횟수 초기화
		}
	}

	return retry <= 10; // 0이면 수신 실패, 1이면 수신 성공
}


// 데이터가 수신되었을 때 수신된 데이터를 처리하는 함수
void Socket::ProcessRecvEvent(SOCKET ah_socket)
{
	// 수신된 데이터를 'Head'와 'Body'로 분리하고 'Head'정보를 분석해서 'MessageID'에 따른 작업을 할 수 있도록 ProcessRecvData 함수를 호출
	// 수신하는 과정에서 끊어 읽기, 재시도 읽기에 의해 FD_READ 이벤트가 과도하게 발생할 수 있으므로
	// 비동기 처리 (WSAASyncSelect)를 이용해 FD_READ 이벤트가 추가로 발생하지 않도록 설정
	// 수신 작업 완료후 FD_READ 이벤트를 수신할 수 있도록 비동기를 다시 걸어준다

	unsigned char msg_id; // msg_id : Body에 저장된 데이터의 종류를 구분 (message_id)
	BS body_size;

	// FD_READ 이벤트가 과도하게 발생하지 않도록 제거 (FD_CLOSE에 의해서만 m_data_notify_id 이벤트가 발생하도록 설정)
	// 이 소켓(ah_socket)에 FD_CLOSE가 발생하면 이 윈도우(mh_notify_wnd)에게 m_data_notify_id 메시지를 준다
	WSAAsyncSelect(ah_socket, mh_notify_wnd, m_data_notify_id, FD_CLOSE); // WSAAsyncSelect : 특정 소켓에 비동기를 건다

	unsigned char key = 0; // key: 구분값. 이 프로토콜이 정상적인 프로토콜인지 체크 (1byte)
	recv(ah_socket, (char*)&key, 1, 0); // 구분값 수신 (ah_socket으로부터 1바이트의 데이터를 key에다가 가져옴)

	// 사용자가 지정한 구분 값과 일치하는지 체크
	if (key == m_valid_key)
	{
		recv(ah_socket, (char*)&msg_id, 1, 0); // Message ID 수신 (1byte)
		recv(ah_socket, (char*)&body_size, sizeof(BS), 0); // Body size 수신 (2byte)

		if (body_size > 0)
		{
			// Body size는 값이 크기 때문에 안정적인 재시도 수신이 가능한 ReceiveData 함수로 데이터를 수신한다
			if (!ReceiveData(ah_socket, body_size)) // ReceiveData : 데이터 수신 실패하면 0반환
			{
				// 데이터를 수신하다 오류가 발생한 경우 연결된 소켓을 해제
				DisconnectSocket(ah_socket, -2);
				return;
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
		DisconnectSocket(ah_socket, -1);
	}

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





// 