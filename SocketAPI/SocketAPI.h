#pragma once

#include <WinSock2.h>

// �������� �����͸� �ְ������ ū�뷮�� �����͸� �� ���� �����ϸ� ���� ���ϸ� ������ �� ������ ����� ���� �ʱ� ������
// �����͸� ����� �����ϰ� �����ϴ� ������ �����͸� �ٽ� ���ļ� ����Ѵ� (Socket class)


// ����, ���� ���� ��� Ŭ����
// ������ �����ͳ� ���ŵ� �����Ϳ� ���� �޸� �Ҵ�, ����
class ExchangeManager 
{
protected:
	int m_total_size, m_current_size; // ���� �Ǵ� ������ ���� �Ҵ�� �޸��� ��ü ũ��� ���� �۾����� ũ��
	char* mp_data; // ���� �Ǵ� ������ ���� �Ҵ�� �޸��� ���� �ּ�

public:
	ExchangeManager(); // ��� ���� �ʱ�ȭ
	~ExchangeManager(); // ����, ������ ���� �Ҵ�� �޸� ����

	char* MemoryAlloc(int a_data_size); // ����, ���ſ� ����� �޸� �Ҵ� (a_data_size�� �Ҵ��� ũ�⸦ ����ϸ� �Ҵ�� �޸� �ּ� ��ȯ)
	void DeleteData(); // ����, ���ſ� ���� �޸� ����

	inline int GetTotalSize() { return m_total_size; } // �Ҵ�� �޸� ũ�� ��ȯ
	inline int GetCurrentSize() { return m_current_size; } // ���� �۾����� �޸� ��ġ ��ȯ
};


// SendManager, RecvManager : ����, Ŭ���̾�Ʈ ���� Ŭ�������� ū ũ���� �����͸� ���� �Ǵ� ������ �� ���


// ���� �� Ŭ����
// ū �����͸� ������ ���� ������ ũ��� �����͸� ����� �����ؾ� �Ѵ�.
// �ϳ��� �޸𸮷� �����ؼ� ������ ������ ���� ���� ��ġ�� �����ϰ� �� ��ġ�� �ڽ��� ���� ũ�⸸ŭ ����
class SendManager : public ExchangeManager
{
public:

	// ������ ��ġ�� ũ�� ��� (���� ���� ��ġ ����) (ū ũ���� �����͸� ������ ���)
	int GetPosition(char** ap_data, int a_data_size = 2048); 
	
	// �� ���۵� ũ��� �Ҵ�� �޸� ��ü ũ�⸦ ���Ͽ� ������ �Ϸ�Ǿ����� üũ (���� �Ϸ�: 1��ȯ) 
	inline int IsProcessing() { return m_total_size == m_current_size; }
};



// ���� �� Ŭ����
// ���ŵ� �������� �� ũ��� �޸𸮸� ����� ���� 
// ���������� ���۵Ǵ� �����͸� �ϳ��� �޸𸮿� ��ġ�� �۾� (������ ��ġ�� �Űܰ��鼭 ���ŵ� �����͸� ����)
// ������ �Ϸ�� ��, �ϳ��� ������ �������� ���� �ּҸ� ��� �Լ�
class RecvManager : public ExchangeManager
{
public:
	int AddData(char* ap_data, int a_size); // ���ŵ� �����͸� ������ ���ŵ� �����Ϳ� �߰� 

	inline char* GetData() { return mp_data; } // ���ŵ� �����͸� �ϳ��� ��ģ �޸��� ���� �ּҸ� ��´�
};



// ��Ʈ��ũ ������ ���� (Head + Body)
// Head 4byte : ���а� 1byte, Message ID 1byte, Body size 2byte
// ���а� : ��Ʈ��ũ�� �����Ͱ� ���ŵǾ��� ��, �� �����Ͱ� ���� ���ϴ� ���������� üũ (1byte)
// Message ID : Body�� ����� �������� ������ ���� (ex. Message ID�� 1�̸� Body�� ����� ���� �α��� ����, 2�̸� ä�� ������) (1byte)
// Body size : Body�� ����� �������� ũ�� (2byte)
// Body : ���α׷��� � �۾��� ���� ������ ����� ������ ���� (ũ�� : Head�� Body size)

typedef unsigned short BS; // Body size (2byte)
#define HEAD_SIZE 2+sizeof(BS) // Head size (key + message_id + body_size)
#define LM_SEND_COMPLETED 29001
#define LM_RECV_COMPLETED 29002



// ����, Ŭ���̾�Ʈ Ŭ������ �ߺ� ����� �����ϴ� Ŭ����
// ������ �����͸� �����ų� ���ŵ� �����͸� �ϳ��� ��ġ�� �۾�
class Socket
{
protected:
	unsigned char m_valid_key; // ���а� (���������� ��ȿ���� üũ�ϱ� ���� ��)
	char* mp_send_data, * mp_recv_data; // ����, ���ſ� ����� �޸�
	HWND mh_notify_wnd; // ������ �ڵ� (���Ͽ� ���ο� �����Ͱ� ���ŵǾ��ų� ���� �����Ǿ��� �� �߻��ϴ� �޽����� ������ ������ �ڵ�)
	int m_data_notify_id; // �����Ͱ� ���ŵǰų� ������� ������ �������� �� ����� �޽��� ID. ���Ͽ� �񵿱� �̺�Ʈ(FD_READ | FD_CLOSE)�� �߻����� �� ������ �ڵ鿡 �Ѱ��� �޽��� ID

public:
	Socket(unsigned char a_valid_key, int a_data_notify_id); // ��ü �����ÿ� �������� ���� ���� ������ ���� �� ���� ������ ����� �޽��� ID ����
	~Socket();


	// ������ ���� �Լ� (���޵� ������ ������ mp_send_data �޸𸮿� ��ӵ� Head ������ �����ؼ� ����)
	int SendFrameData(SOCKET ah_socket, unsigned char a_message_id, const char* ap_body_data, BS a_body_size);

	// �������� ������ ���� (��õ� ����)
	int ReceiveData(SOCKET ah_socket, BS a_body_size);

	// �����Ͱ� ���ŵǾ��� �� ���ŵ� �����͸� ó���ϴ� �Լ�
	void ProcessRecvEvent(SOCKET ah_socket);


	// DisconnectSocket, ProcessRecvData �Լ��� ���� �������� Ŭ���̾�Ʈ ���������� ���� ������ �޶��� �� �����Ƿ� 
	// �����Լ��� �����ϰ� ����ϴ� Ŭ�������� �������Ͽ� ���
	
	// ���ӵ� ����� ���� �Լ� (�ڽ� Ŭ�������� ������)	
	virtual void DisconnectSocket(SOCKET ah_socket, int a_error_code) = 0; 
	// ���ŵ� �����͸� ó���ϴ� �Լ� (�ڽ� Ŭ�������� ������)
	virtual int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size) = 0; 


	// IP �ּ��� ���ڿ� ���� ��ȯ �Լ� (ASCII 1byte, �����ڵ� 2byte)
	static void AsciiToUnicode(wchar_t* ap_dest_ip, char* ap_src_ip); // ASCII ������ ���ڿ��� �����ڵ�� ��ȯ
	static void UnicodeToAscii(char* ap_dest_ip, wchar_t* ap_src_ip); // �����ڵ� ������ ���ڿ��� ASCII�� ��ȯ
};



// ����� ���� ������ Ŭ���� (�ϳ��� ����� ������ �����ϱ� ���� Ŭ����)
// ������ ���Ͽ��� ���ӵ� Ŭ���̾�Ʈ�� �����ϱ� ���� ���
class UserData
{
protected:
	// Ŭ���̾�Ʈ�� ����ϱ� ���� ����� ���� �ڵ� 
	SOCKET mh_socket; // Ŭ���̾�Ʈ ������ ������ �õ��ϸ� ������ accept�Լ��� ���� �ش� Ŭ���̾�Ʈ�� ����� ���ο� ���� �ڵ��� �����

	// Ŭ���̾�Ʈ���� ū �����͸� �����ϱ� ���� ����� ��ü
	SendManager* mp_send_man; // mh_socket�� ����Ǿ� �ִ� Ŭ���̾�Ʈ ���Ͽ� ū ũ���� �����͸� ������ �� ��� 

	// Ŭ���̾�Ʈ���Լ� ū �����͸� �����ϱ� ���� ����� ��ü
	RecvManager* mp_recv_man; // mh_socket�� ����Ǿ� �ִ� Ŭ���̾�Ʈ �������κ��� ū ũ���� �����͸� ������ �� ��� 

	wchar_t m_ip_address[16]; // ������ Ŭ���̾�Ʈ�� IP �ּ�

public:
	UserData(); // ������� �ʱ�ȭ, ���۰� ���ſ� ����� ��ü ����
	~UserData(); // ����ϴ� ���� ����, ���۰� ���ſ� ����� ��ü ����


	// ��������� ���� Ŭ���� �ܺο��� ����� �� �ֵ��� ���� �������̽� �Լ���
	inline SOCKET GetHandle() { return mh_socket; }
	inline void SetHandle(SOCKET ah_socket) { mh_socket = ah_socket; }
	inline SendManager* GetSendMan() { return mp_send_man; }
	inline RecvManager* GetRecvMan() { return mp_recv_man; }
	inline wchar_t* GetIP() { return m_ip_address; }
	inline void SetIP(const wchar_t* ap_ip_address) { wcscpy(m_ip_address, ap_ip_address);  }


	// ���� ���� Ŭ�������� Ŭ���̾�Ʈ�� ������ ������ �� ������ �ݰ� �ʱ�ȭ�ϴ� �۾��� �ؾ� �ϴµ� Ŭ���̾�Ʈ�� ���� �ڵ���
	// �� Ŭ������ ������ �־ �Ź� GetHandle, SetHandle �Լ��� �ݺ������� ����ؾ� �ϴ� �������� �ִ�
	// �׷��� �Ʒ��� ���� CloseSocket �Լ��� �߰��� �����Ѵ�
	// DisconnectSocket ���� CloseSocket ���� ���� ���� ������?
	void CloseSocket(int a_linger_flag); // ����� ������ �ݰ� �ʱ�ȭ


	// ������ ���� ��, ������ Ŭ������ Ȯ���� �� �� �Լ��� ����Ѵ� (UserAccount)
	virtual UserData* CreateObject() { return new UserData; }
};



// �α��� ������ ����ϱ� ���� ���̵� ��ȣ ���� ������ �߰��� ������ �ʿ䰡 �ִٸ�
// UserData���� ��ӹ��� ���ο� Ŭ���� UserAccount�� ���� ���
// UserAccount Ŭ������ UserData���� ��ӹ޾ұ� ������ �������� �����ϸ� UserAccount�� ������� ��ü�� �ּҸ� UserDataŬ������ �����ͷ� ����� �� �ִ�
// ���� ������ ������ UserDataŬ������ �����͸� ����ؼ� ���α׷��� �Ǿ� �ִ��� 
// UserAccount�� �޸𸮸� �����Ҵ��ؼ� �Ѱ��ָ� ������ ���� ���������δ� UserAccountŬ������ ������� ��ü�� ���ȴ�
class UserAccount : public UserData
{
protected:
	wchar_t id[32]; // ������� ���̵�
	wchar_t password[32]; // ������� ��й�ȣ

public:
	wchar_t* GetID() { return id; }
	void SetID(const wchar_t* ap_id) { wcscpy(id, ap_id); }
	wchar_t* GetPassword() { return password; }
	void SetPassword(const wchar_t* ap_password) { wcscpy(password, ap_password); }


	// UserAccount Ŭ������ ������ ������ ����� ���� ���߿� ����ڰ� �ʿ��ؼ� �����ϴ� ���̱� ������ ������ ������ ����� �������� UserAccount Ŭ������ ����
	// �� �κ��� �ذ��ϱ� ���� CreateObject �Լ� ���

	// ������ ���� ��, ������ Ŭ������ Ȯ���� �� �� �Լ��� ����Ѵ�
	virtual UserData* CreateObject() { return new UserAccount; }
	// CreateObject �Լ��� �߰��Ǹ� �������� ����� �ҽ����� UserAccountŬ������ �𸣴��� CreateObject�Լ��� ����ϸ� ���� ����ϴ� ��ü�� ������ ��ü�� ������ �� �ִ�
	// �̰��� �����Ϸ��� ���� Ŭ���� ���� �ÿ� new UserAccount���� ���ڷ� �Ѱܼ� ����� ������ ������ Ŭ������ �����ؼ� �Ѱ���� �ȴ�

	// ex)
	// ServerSocket my_server(0x27, MAX_USER_COUNT, new UserAccount); // ����� ������ UserAccount�� ����ϰ� �ʹٸ�
	// ServerSocket my_server(0x27, MAX_USER_COUNT, new UserData);    // ����� ������ UserData�� ����ϰ� �ʹٸ�
};



// ���ӵ� ����� ���� -> UserData Ŭ������ �ϳ��� ����� ������ �����ϱ� ���� Ŭ�����̱� ������ 
// �� Ŭ������ ����Ͽ� ���� ���� ����ڸ� ������ �� �ֵ��� �����ؾ� �Ѵ� -> ������ ���� Ŭ�������� ����
// [���� ���� Ŭ�������� ����� �����ϴ� �κ��� ������ Ŭ������ �и��ؼ� �ڵ�� �����غ���]



// 