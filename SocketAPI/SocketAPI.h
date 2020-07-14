#pragma once

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



// Socket 클래스 - 전송할 데이터를 나누거나 수신된 데이터를 하나로 합치는 작업을 담당할 클래스 