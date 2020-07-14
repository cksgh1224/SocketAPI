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





// 