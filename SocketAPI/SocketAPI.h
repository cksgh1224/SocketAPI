#pragma once

#include "pch.h"
#include "framework.h"
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
	
	// �� ���۵� ũ��� �Ҵ�� �޸� ��ü ũ�⸦ ���Ͽ� ������ �Ϸ�Ǿ����� üũ
	inline int IsProcessing() { return m_total_size != m_current_size; } // ������ �����Ͱ� ���������� 1��ȯ (������)
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
#define LM_SEND_COMPLETED 29001 // ��뷮 �����Ͱ� ���� �Ϸ� �Ǿ��� �� ����� �޽��� (�����쿡�� ������ �Ϸ�Ǿ����� �˸��� �ʹٸ� LM_SEND_COMPLETED �޽����� ���)
#define LM_RECV_COMPLETED 29002 // ��뷮 �����Ͱ� ���� �Ϸ� �Ǿ��� �� ����� �޽��� (�����쿡�� ������ �Ϸ�� �����͸� ����Ϸ��� LM_RECV_COMPLETED �޽����� ���)
// a_accept_notify_id = 25001 // FD_ACCEPT �߻��� �����쿡 ������ �޽��� ID
// a_data_notify_id = 25002   // FD_READ, FD_CLOSE �߻��� �����쿡 ������ �޽��� ID



// ����, Ŭ���̾�Ʈ Ŭ������ �ߺ� ����� �����ϴ� Ŭ����
// ������ �����͸� �����ų� ���ŵ� �����͸� �ϳ��� ��ġ�� �۾�
class Socket
{
protected:
	unsigned char m_valid_key; // ���а� (���������� ��ȿ���� üũ�ϱ� ���� ��)
	char* mp_send_data, * mp_recv_data; // ����, ���ſ� ����� �޸�
	HWND mh_notify_wnd; // ������ �ڵ� (���Ͽ� ���ο� �����Ͱ� ���ŵǾ��ų� ���� �����Ǿ��� �� �߻��ϴ� �޽����� ������ ������ �ڵ�)
	int m_data_notify_id; // �����Ͱ� ���ŵǰų� ������� ������ �������� �� ����� �޽��� ID (FD_READ, FD_CLOSE �̺�Ʈ�ÿ� �߻��� �޽���) (������ �ڵ鿡 �Ѱ��� �޽��� ID)


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



// ������ ���� Ŭ����
// ������ listen (Ŭ���̾�Ʈ�� ���� ��û ��� ) ����� �����ϴ� �۾��� ���� ���� �ؾ� �Ѵ�.
// listen �۾��� ���۵Ǹ� ���ο� ����ڵ��� ������ �ϰ� �ǰ� ������ ����� ���ÿ� ����� ���� ������ �ʿ��ϴ�.
// ������ ����ڵ�� ������ �������ݷ� ����� �ϸ鼭 ������ ������ �� ���� ��Ʈ��ũ �۾��� ���ӵȴ�
class ServerSocket : public Socket
{
protected:
	SOCKET mh_listen_socket; // listen �۾��� ����� ���� �ڵ� (Ŭ���̾�Ʈ�� ������ �޾��ִ� ����)
	int m_accept_notify_id; // ���ο� Ŭ���̾�Ʈ�� �������� �� �߻��� �޽��� ID �� (FD_ACCEPT �̺�Ʈ�ÿ� �߻��� �޽��� ID)
	unsigned short m_max_user_count; // ������ ������ �ִ� ����� �� (������ ���� ������ �ִ� ����� �� - �ִ� 65535��)
	UserData** mp_user_list; // �ִ� ����ڸ� �����ϱ� ���ؼ� ����� ��ü�� (���� ������)

	// UserData** mp_user_list;  ->  ��ü �����ڸ� ���� ����� �� �ִ� ���·� ����� ���� ���� ������ ���
	// ��ü �����ڿ��� mp_user_list�� �����ϴ� �ִ� ����� ����ŭ �޸𸮸� �Ҵ��ؾ� �ϴµ� �Ʒ��� ���� ������ �����͸� ����ؼ� ������ ���� �ִ�
	// UserData* mp_user_list = new UserData[m_max_user_count];
	// �̷��� �����ϸ� ����� ������ ������ ��ü�� UserData�� �����Ǿ� ������ ������ ������ ������ ����µ� ������ �����

public:
	ServerSocket(unsigned char a_valid_key, unsigned short a_max_user_count, UserData* ap_user_data, int a_accept_notify_id = 25001, int a_data_notify_id = 25002);
	virtual ~ServerSocket(); // ���� �ı���

	// ���� �ı��ڰ� �ʿ��� ��� : ����, �Ļ� Ŭ�������� �ı��ڸ� ������ ���
	// ���� Ŭ������ �ı��ڰ� ������ �ƴ϶�� ���������� �ش��ϴ�(���� Ŭ����) �ı��ڸ� ȣ��ȴ� (��ü���� �ش��ϴ� �ı��ڰ� ȣ����� �ʴ´�)
	// BŬ������ A�κ��� ��ӹް� A,B Ŭ���� ��� �ı��ڰ� �ִ� ���¿��� AŬ������ �ı��ڰ� ������ �ƴ϶��
	// A* test = new B(); delete test; -> A�� �ı��ڸ� ȣ�� (B�� �ı��ڴ� ȣ����� �ʴ´�)


	// ���� ������ ���� (listen �۾��� ������ �Լ�)
	int StartServer(const wchar_t* ap_ip_address, int a_port, HWND ah_notify_wnd);

	// Ŭ���̾�Ʈ�� ���� ó�� (FD_ACCEPT ó�� �Լ�)
	int ProcessToAccept(WPARAM wParam, LPARAM lParam);


	// �߰��۾�
	virtual void AddWorkForAccept(UserData* ap_user) = 0; // Accept �ÿ� �߰������� �ؾ��� �۾��� �ִٸ� �� �Լ��� �������̵��ؼ� ó��	
	virtual void ShowLimitError(const wchar_t* ap_ip_address) = 0; // �ִ� ����ڼ� �ʰ��ÿ� �߰������� �ؾ��� �۾��� �ִٸ� �� �Լ��� �������̵��ؼ� ó��


	// Ŭ���̾�Ʈ�� ��Ʈ��ũ �̺�Ʈ ó�� (FD_READ, FD_CLOSE ó�� �Լ�)
	void ProcessClientEvent(WPARAM wParam, LPARAM lParam);


	// Ŭ���̾�Ʈ ���� �����ÿ� �߰����� �۾��� �ʿ��ϴٸ� ��ӹ��� Ŭ�������� �� �Լ��� �������ؼ� ���
	// a_error_code : 0�̸� ��������, -1�̸� Ű���� ��ȿ���� �ʾƼ� ����, -2�̸� �ٵ����� �����߿� ���� �߻�
	virtual void AddWorkForCloseUser(UserData* ap_user, int a_error_code) = 0;
	

	// ���� ��Ʈ��ũ �̺�Ʈ�� ���� �޽����� �߻��ϸ�, �ش� �̺�Ʈ�� �߻��� ������ �ڵ��� �޽����� wParam�׸����� ���޵ȴ�
	// ������ ���� ���� ����ڰ� ���ÿ� �����ؼ� ����ϱ� ������ �ش� ������ � ������� �������� �� �� ����
	
	// ���� �ڵ��� ����Ͽ� � ��������� ã�´� (ã���� ������� ��ġ�� ��ȯ)
	inline int FindUserIndex(SOCKET ah_socket)
	{
		for (int i = 0; i < m_max_user_count; i++)
			if (mp_user_list[i]->GetHandle() == ah_socket) return i;

		return -1;
	}

	// ���� �ڵ��� ����Ͽ� � ��������� ã�´� (ã���� ����� ������ �����ϴ� ��ü�� �ּҸ� ��ȯ)
	inline UserData* FindUserData(SOCKET ah_socket)
	{
		for (int i = 0; i < m_max_user_count; i++)
			if (mp_user_list[i]->GetHandle() == ah_socket) return mp_user_list[i];

		return NULL;
	}


	// Ŭ���̾�Ʈ�� ������ ������ �����ϱ� (����� ������ �ݰ� �ʱ�ȭ) (UserData::CloseSocket ���)
	void DisconnectSocket(SOCKET ah_socket, int a_error_code);


	// ���ŵ� �����͸� ó���ϴ� �Լ�
	int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size);


	// �������� �����ϴ� ��ü ����ڿ� ���� ������ �ִ� ����� ���� �ܺο��� �̿��� �� �ֵ��� ���ִ� �Լ�
	inline UserData** GetUserList() { return mp_user_list; } // ��ü ����ڿ� ���� ����
	unsigned short GetMaxUserCount() { return m_max_user_count; } // �ִ� ����� ��
};



// Ŭ���̾�Ʈ�� ���� Ŭ����
// ������ �����ؼ� ���񽺸� �����޴� �����̱� ������ ������ �Ѱ��� ����ص� ����� �۾��� ����
// ������ ������ �õ��� �� �����ð��� ���� �� �ְ� ������ ������ ���� �ֱ� ������ ���� ���¸� ������ ������ �־�� �Ѵ�
// connect �Լ��� ����ؼ� ������ ������ �õ��ϸ� ������ ������ ���� ��� ������� ���¿� ���� �� �����Ƿ� �񵿱� ����
// ���� ���ӿ� ���� ��� �̺�Ʈ�� FD_CONNECT�� �߻��ϸ� Ŭ���̾�Ʈ ������ ����ϴ� �����쿡 �޽��� ����
// ������ ū �뷮�� �����͸� �����ϰų� �����κ��� ū �뷮�� �����͸� ���� ������ ����� ��ü�� ������ �־�� �Ѵ�
class ClientSocket : public Socket
{
protected:
	SOCKET mh_socket; // ������ ����ϱ� ���� ����� ���� �ڵ�
	char m_connect_flag; // 0: ���� �ȵ�, 1: ���� �õ���, 2: ������
	int m_connect_notify_id; // ������ ������ �õ��� ����� �˷��� ������ �޽��� ID (FD_CONNECT)
	SendManager m_send_man; // ������ ū �����͸� �����ϱ� ���� ����� ��ü
	RecvManager m_recv_man; // ������ ū �����͸� �����ϱ� ���� ����� ��ü

public:
	ClientSocket(unsigned char a_valid_key, int a_connect_notify_id, int a_data_notify_id);
	~ClientSocket();


	// ������ �����ϱ�
	int ConnectToServer(const wchar_t* ap_ip_address, int a_port_num, HWND ah_notify_wnd); 

	// ���� �õ��� ���� ��� ó���ϱ� (FD_CONNECT)
	int ResultOfConnection(LPARAM lParam); 

	// ������ ���� ó���� ���� ���� ������ ���� ó�� (FD_READ, FD_CLOSE)
	int ProcessServerEvent(WPARAM wParam, LPARAM lParam); 

	// ������ ������ ������ �����ϱ�
	void DisconnectSocket(SOCKET ah_socket, int a_error_code);

	
	// ������ ���� �Լ� (���޵� ������ ������ mp_send_data �޸𸮿� ��ӵ� Head ������ �����ؼ� ����)
	int SendFrameData(unsigned char a_message_id, const char* ap_body_data, BS a_body_size); // Socket::SendFrameData �����ε�

	// ���ŵ� �����͸� ó���ϴ� �Լ�
	int ProcessRecvData(SOCKET ah_socket, unsigned char a_msg_id, char* ap_recv_data, BS a_body_size);


	 
	inline int IsConnected() { return m_connect_flag == 2; } // �������� ���ӻ��¸� �˰� ������ ��� (��ȯ�� 0:��������, 1: ���ӻ���)  ���������δ� ���¸� �������� ���������� �ܺο� �˷��ٶ��� �ΰ��� ���·� �˷��ش� ('���� �õ���' ���´� ������ �����Ѵ�)
	inline SOCKET GetHandle() { return mh_socket; } // ������ ����ϱ� ���� ������ ������ �ڵ� ���� �˰� ���� �� ���
};