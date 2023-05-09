#pragma once

#include <WinSock2.h>
#include "types.h"


MySocketError InitMySocket(MySocket* my_sock, int af, int type, int protocol, LPSTR ip, u_short port) {
	struct in_addr addres_ip;
	
	if (InetPton(af, ip, &addres_ip) != 1)
		return MYSOCKER_INVALID_IP;
	if (port >= USHRT_MAX || port < 1)
		return MYSOCKER_INVALID_PORT;
	
	my_sock->socket = socket(af, type, protocol);
	memset(&my_sock->addr_in, 0, sizeof(my_sock->addr_in));
	my_sock->addr_in.sin_family = af;
	my_sock->addr_in.sin_addr.s_addr = addres_ip.s_addr;
	my_sock->addr_in.sin_port = htons(port);

	if (bind(my_sock->socket, (struct sockaddr*)&my_sock->addr_in, sizeof(my_sock->addr_in)) == SOCKET_ERROR)
	{
		return WSAGetLastError();
	}

	return MYSOCKER_OK;
}

MySocketError InitMySocketClient(MySocket* my_sock, int af, int type, int protocol) {
	my_sock->socket = socket(af, type, protocol);
	memset(&my_sock->addr_in, 0, sizeof(my_sock->addr_in));

	if (my_sock->socket == INVALID_SOCKET)
		return MYSOCKER_INVALID_SOCKET;

	return MYSOCKER_OK;
}

MySocketError InitMySocketConnectedServer(MySocket* server_sock, int af, int type, int protocol, LPSTR ip, u_short port) {
	struct in_addr addres_ip;
	
	if (InetPton(af, ip, &addres_ip) != 1)
		return MYSOCKER_INVALID_IP;
	if (port >= USHRT_MAX || port < 1)
		return MYSOCKER_INVALID_PORT;

	memset(&server_sock->socket, 0, sizeof(server_sock->socket));
	server_sock->addr_in.sin_family = af;
	server_sock->addr_in.sin_addr.s_addr = addres_ip.s_addr;
	server_sock->addr_in.sin_port = htons(port);

	return MYSOCKER_OK;
}

void SetZeroMySocket(MySocket* my_sock) {
	memset(my_sock, 0, sizeof(*my_sock));
}

MySocketError ListenMySocket(MySocket* my_sock, int max_conn) {
	if (listen(my_sock->socket, max_conn) == SOCKET_ERROR)
		return WSAGetLastError();

	return MYSOCKER_OK;
}

MySocketError ConnectMySocket(MySocket* server_socket, MySocket* client_socket) {
	if (connect(client_socket->socket, (struct sockaddr*)&server_socket->addr_in, sizeof(server_socket->addr_in)) == SOCKET_ERROR)
		return WSAGetLastError();

	return MYSOCKER_OK;
}

MySocketError AcceptMySocket(MySocket* server_socket, MySocket* client_socket) {
	int client_addr_size = sizeof(client_socket->addr_in);
	client_socket->socket = accept(server_socket->socket, (struct sockaddr*)&client_socket->addr_in, &client_addr_size);

	if (client_socket->socket == INVALID_SOCKET)
		return MYSOCKER_ACCEPT_ERROR;

	return MYSOCKER_OK;
}

MySocketError RecvMySocket(MySocket* connected_socket, LPSTR buffer, int buffer_Size, int* recv_size, int flags) {
	*recv_size = recv(connected_socket->socket, buffer, buffer_Size, flags);

	if (*recv_size > 0)
		return MYSOCKER_OK;
	else if (*recv_size == 0)
		return MYSOCKER_CLIENT_HAS_DISCONNECTED;
	else {
		*recv_size = 0;
		return WSAGetLastError();
	}
}

MySocketError SendMySocket(MySocket* client_socket, LPSTR buffer, int buffer_size, int* send_size, int flags) {
	*send_size = send(client_socket->socket, buffer, buffer_size, flags);

	if (*send_size >= 0)
		return MYSOCKER_OK;
	else
	{
		*send_size = 0;
		return WSAGetLastError();
	}
}

MySocketError SendMySocketPartial(MySocket* client_socket, LPCSTR buffer, int buffer_size, int flags) {
	int chunk_size = 65536;
	int bytes_sent = 0;
	for (int i = 0; i < buffer_size; i += chunk_size) {
		int bytes_to_send = min(chunk_size, buffer_size - i);
		int bytes = send(client_socket->socket, buffer + i, bytes_to_send, flags);

		if (bytes == SOCKET_ERROR) {
			return WSAGetLastError();
		}

		bytes_sent += bytes;
	}

	return MYSOCKER_OK;
}

MySocketError RecvMySocketPartial(MySocket* client_socket, LPSTR buffer, int buffer_size, int flags) {
	int chunk_size = 65536;
	int bytes_received = 0;

	for (int i = 0; i < buffer_size; i += chunk_size) {
		int bytesToReceive = min(chunk_size, buffer_size - i);
		int bytes = recv(client_socket->socket, buffer + i, bytesToReceive, 0);

		if (bytes == SOCKET_ERROR) {
			return WSAGetLastError();
		}

		bytes_received += bytes;

		if (bytes == 0) {
			return MYSOCKER_OK;
		}
	}

	return MYSOCKER_OK;
}

MySocketError CloseMySocket(MySocket* my_sock) {
	if (closesocket(my_sock->socket) == SOCKET_ERROR)
		return WSAGetLastError();
	else {
		memset(my_sock, 0, sizeof(*my_sock));
	}

	return MYSOCKER_OK;
}

int GetMySocketErrorMessage(MySocketError code, LPSTR buffer) {
	switch (code)
	{
	case MYSOCKER_OK: return wsprintf(buffer, L"Код %d. Ошибок нет.", code);
	case MYSOCKER_INVALID_IP: return wsprintf(buffer, L"Код %d. Невалидный ip-адрес.", code);
	case MYSOCKER_INVALID_PORT: return wsprintf(buffer, L"Код %d. Невалидный порт.", code);
	case MYSOCKER_NOT_INITIALIZED: return wsprintf(buffer, L"Код %d. Сокет не инициализирован.", code);
	case MYSOCKER_ACCEPT_ERROR: return wsprintf(buffer, L"Код %d. Ошибка подключения клиента.", code);
	case MYSOCKER_CLIENT_HAS_DISCONNECTED: return wsprintf(buffer, L"Код %d. Клиент отсоединился.", code);
	default: return wsprintf(buffer, L"Ошибка WinSock2, связанная с сокетами. Код %d", code);
	}
}

BOOL MySocketMessageIfError(HWND hWnd, MySocketError code) {
	if (code != MYSOCKER_OK)
	{
		WCHAR buffer[512];
		GetMySocketErrorMessage(code, buffer);
		MessageBox(hWnd, buffer, L"Ошибка", MB_OK);
		return TRUE;
	}
	return FALSE;
}