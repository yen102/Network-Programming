// WSAAsyncSelectServer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Server.h"
#include "winsock2.h"
#include "windows.h"
#include "stdio.h"
#include "conio.h"
#include "ws2tcpip.h"
#include "time.h"

#define WM_SOCKET WM_USER + 1
#define SERVER_PORT 6000
#define MAX_CLIENT 1024
#define SIZE 2048
#define SERVER_ADDR "127.0.0.1"

#define EXCEPTION "00"
#define LOG_OK "10"
#define LOG_FAIL "11"
#define POST_OK "20"
#define POST_FAIL "21"
#define QUIT_OK "30"
#define QUIT_FAIL "31"

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

struct clientST {
	char clientIP[INET_ADDRSTRLEN];
	int clientPort;
	SOCKET sock;
	int status; // loged in or not
};

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	windowProc(HWND, UINT, WPARAM, LPARAM);

struct clientST clients[MAX_CLIENT];
SOCKET listenSock;

int Receive(SOCKET sock, char* buf, int len, int flag);
int Send(SOCKET sock, char* buf, int len, int flag);
void handleClient(struct clientST* client);
void handleClient(struct clientST*);
const char* handleLogIn(struct clientST* client, char* buf);
const char* handlePost(struct clientST* client);
const char* handleQuit(struct clientST* client, int quitType);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	MSG msg;
	HWND serverWindow;

	//Registering the Window Class
	MyRegisterClass(hInstance);

	//Create the window
	if ((serverWindow = InitInstance(hInstance, nCmdShow)) == NULL)
		return FALSE;

	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		MessageBox(serverWindow, L"Winsock 2.2 is not supported.", L"Error!", MB_OK);
		return 0;
	}

	//Construct socket	
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//requests Windows message-based notification of network events for listenSock
	WSAAsyncSelect(listenSock, serverWindow, WM_SOCKET, FD_ACCEPT | FD_CLOSE | FD_READ);

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		MessageBox(serverWindow, L"Cannot associate a local address with server socket.", L"Error!", MB_OK);
	}

	//Listen request from client
	if (listen(listenSock, MAX_CLIENT)) {
		MessageBox(serverWindow, L"Cannot place server socket in state LISTEN.", L"Error!", MB_OK);
		return 0;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int i;
	for (i = 0; i <MAX_CLIENT; i++)
		clients[i].sock = 0;
	hWnd = CreateWindow(L"WindowClass", L"WSAAsyncSelect TCP Server", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_SOCKET	- process the events on the sockets
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SOCKET client;
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr), i;
	int clientPort;
	char clientIP[INET_ADDRSTRLEN];
	switch (message) {
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			for (i = 0; i < MAX_CLIENT; i++)
				if (clients[i].sock == (SOCKET)wParam) {
					closesocket(clients[i].sock);
					clients[i].sock = 0;
					continue;
				}
		}

		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
		{
			client = accept((SOCKET)wParam, (sockaddr *)&clientAddr, &clientAddrLen);
			if (client == INVALID_SOCKET) {
				break;
			}
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);

			/* assign value to client */
			struct clientST new_client;
			new_client.clientPort = clientPort;
			strcpy(new_client.clientIP, clientIP);
			new_client.sock = client;
			for (i = 0; i < MAX_CLIENT; i++)
				if (clients[i].sock == 0) {
					clients[i] = new_client;
					break;
					//requests Windows message-based notification of network events for listenSock
					WSAAsyncSelect(clients[i].sock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
				}
			if (i == MAX_CLIENT)
				MessageBox(hWnd, L"Too many clients!", L"Notice", MB_OK);
		}
		break;

		case FD_READ:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if (clients[i].sock == (SOCKET)wParam)
					break;
			handleClient(&clients[i]);
		}
		break;

		case FD_CLOSE:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if (clients[i].sock == (SOCKET)wParam) {
					closesocket(clients[i].sock);
					clients[i].sock = 0;
					break;
				}
		}
		break;
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;

	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
void handleClient(struct clientST* client) {
	char buf[SIZE];
	memset(buf, 0, SIZE);
	int received;
	char len_str[8]; // store len of incoming request in ascii form
	memset(len_str, 0, 8);
	received = Receive(client->sock, len_str, 4, 0);

	//received = recv(client->sock, buf, SIZE, 0);
	int len = atoi(len_str); // convert len to integer
	len = ntohs(len); // convert to host order
	received = Receive(client->sock, buf, len, 0); // receive len-bytes data
	char response[4]; // store response to send to client
	memset(response, 0, 4);
	FILE* fp = fopen("log_20183862.txt", "a+"); // open log file
	if (fp == NULL) {
		printf("[-] Cannot open log file!\n");
	}
	char timeBuf[128]; // contain time information
	memset(timeBuf, 0, 128);
	struct tm *t;
	time_t now;

	if (strncmp(buf, "USER", 4) == 0) { // client log in
		strcpy(response, handleLogIn(client, buf));
		Send(client->sock, response, 4, 0);
	}
	else if (strncmp(buf, "POST", 4) == 0) { // client post message
		strcpy(response, handlePost(client));
		Send(client->sock, response, 4, 0);
	}
	else if (strncmp(buf, "QUIT", 4) == 0) { // client chose quit mode
		int quitType = 1;
		strcpy(response, handleQuit(client, quitType));
		Send(client->sock, response, 4, 0);
	}
	else { // exceptional requests
		if (strlen(buf) > 0) {
			strcpy(response, EXCEPTION);
			Send(client->sock, response, 4, 0);
		}
	}
	/* write log to file */
	time(&now);
	t = localtime(&now);
	strftime(timeBuf, 128, "%d/%m/%Y %X", t);
	fputs(client->clientIP, fp);
	fputs(":", fp);
	fprintf(fp, "%d", client->clientPort);
	fputs(" [", fp);
	fputs(timeBuf, fp);
	fputs("] $ ", fp);
	fputs(buf, fp);
	fputs(" $ ", fp);
	fputs(response, fp);
	fputs("\r\n", fp);
	fclose(fp);
}

/*Handle log in from client
Input: clientST* client: client communicating with server at the moment
Output: LOG_OK on success else LOG_FAIL
*/
const char* handleLogIn(struct clientST* client, char* buf) {
	FILE* fp = fopen("account.txt", "r+");
	if (fp == NULL) {
		printf("[-] Cannot open file!\n");
	}
	char username[SIZE];
	strncpy(username, buf + 4, SIZE); // username start from 4th byte (first 4 bytes are "USER")
	char tmp[SIZE];
	if (client->status == 1) return LOG_FAIL; // if client already loged in
	while (fgets(tmp, SIZE, fp)) {
		if (strncmp(username, tmp, strlen(username)) == 0) { // if found matched username in account file
			if (tmp[strlen(username) + 1] == '0') { // and not blocked
				client->status = 1; // change status of client to loged in
				fclose(fp);
				return LOG_OK;
			}
		}
	}
	fclose(fp);
	return LOG_FAIL;
}

/* Handle quit from client with quitType:
Input:
param: quitType 
	quitType = -1 indicates client close program or hit ctrl + c
	quitType = 1 indicates client choose quit mode from program
param: clientST* client: client communicating with server at the moment

Output: QUIT_OK on success else QUIT_FAIL
*/
const char* handleQuit(struct clientST* client, int quitType) {
	if (quitType == -1) {
		closesocket(client->sock);
		client->sock = 0;
		return QUIT_OK;
	}
	else if (quitType == 1) {
		if (client->status == 1) {
			client->status = 0;
			return QUIT_OK;
		}
	}
	return QUIT_FAIL;
}

/*Handle post from client 
Input: clientST* client: client communicating with server at the moment
Output: POST_OK on success else POST_FAIL
*/
const char* handlePost(struct clientST* client) {
	if (client->status == 1) return POST_OK; // if client already logged in
	return POST_FAIL;
}

/* The send() wrapper function*/
int Send(SOCKET sock, char* buf, int len, int flag) {
	int sent; // number of bytes have been sent
	int left; // number of bytes left
	left = len;
	while (left) {
		sent = send(sock, buf, left, flag);
		if (sent == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot send data to client !.\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		left -= sent;
		buf += sent;
	}
	return len;
}

/* The recv() wrapper function */
int Receive(SOCKET sock, char* buf, int len, int flag) {
	int received; // number of bytes have been received
	int left; // number of bytes left
	left = len;
	while (left) {
		received = recv(sock, buf, left, flag);
		if (received == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot receive data from client !.\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		left -= received;
		buf += received;
	}
	return len;
}
