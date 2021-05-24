// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "ws2tcpip.h"

#define SIZE 2048

#define LOG_OK "10"
#define LOG_FAIL "11"
#define POST_OK "20"
#define POST_FAIL "21"
#define QUIT_OK "30"
#define QUIT_FAIL "31"

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

int Receive(SOCKET sock, char* buf, int len, int flag);
int Send(SOCKET sock, char* buf, int len, int flag);

void displayChoice() {
	printf("---------------------------------\n");
	printf("%s", "1. Log in\n");
	printf("%s", "2. Post message\n");
	printf("%s", "3. Quit\n");
	printf("%s", "Enter your choice: ");
}

/* Handle log in action of client*/
void handleLogIn(SOCKET client) {
	char username[SIZE]; // store client's username
	char buf[SIZE]; 
	char responseFromServer[4]; // store response from server

	memset(buf, 0, SIZE);
	strcat(buf, "USER"); // indicate login mode

	printf("Username: ");
	memset(username, 0, SIZE);
	gets_s(username, SIZE);
	printf("\n");
	strcat(buf, username);

	int len = strlen(buf); // len of data to send to server
	char len_str[8]; // store len of data converted to ascii form
	itoa(htons(len), len_str, 10); // convert len to ascii, base 10 with network order

	Send(client, len_str, 8, 0); // send len to server
	Send(client, buf, len, 0); // send data

	int received = recv(client, responseFromServer, 4, 0); 
	if (strncmp(responseFromServer, LOG_OK, 2) == 0) {
		printf("Successfully loged in!\n");
	}
	else {
		printf("Failed to log in! Please try again!\n");
	}
}

/* Handle post action of client */
void handlePost(SOCKET client) {
	char buf[SIZE];
	memset(buf, 0, SIZE);
	char responseFromServer[4]; // store respens from server
	strcat(buf, "POST"); // indicate posting mode
	
	printf("Message: ");
	char message[SIZE]; // store message client want to post
	memset(message, 0, SIZE);
	gets_s(message, SIZE);
	printf("\n");
	strcat(buf, message);

	int len = strlen(buf); // len of data to send to server
	char len_str[8]; // store len of data converted to ascii form
	itoa(htons(len), len_str, 10); // convert len to ascii, base 10 with network order

	Send(client, len_str, 8, 0); // send len to server
	Send(client, buf, len, 0); // send data

	int received = recv(client, responseFromServer, 4, 0);
	if (strncmp(responseFromServer, POST_OK, 2) == 0) {
		printf("Successfully post message!\n");
	}
	else {
		printf("Failed to post messasge! Please try again!\n");
	}
}

void handleQuit(SOCKET client) {
	char buf[SIZE]; memset(buf, 0, SIZE);
	strcat(buf, "QUIT");
	char responseFromServer[4];

	int len = strlen(buf);
	char len_str[8]; // store len of data converted to ascii form
	itoa(htons(len), len_str, 10); // convert len to ascii, base 10 with network order

	Send(client, len_str, 8, 0); // send len to server
	Send(client, buf, len, 0); // send data

	int received = recv(client, responseFromServer, 4, 0);
	if (strncmp(responseFromServer, QUIT_OK, 2) == 0) {
		printf("Quited!\n");
	}
	else {
		printf("Failed to quit!\n");
	}
}
int main(int argc, char** argv)
{
	// Get input
	if (argc != 3) {
		printf("usage: client.exe ServerIPAddress ServerPortNumber\n");
		return 0;
	}
	int serverPort = atoi(argv[2]);
	char* serverIP = argv[1];
	// Initiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("[-] Winsock 2.2 is not supported!\n");
		return 0;
	}

	// Construct sockets
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("[-] Error %d: Cannot create client socket!\n", WSAGetLastError());
		return 0;
	}
	printf("[+] Successfully created client socket!\n");

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

	// Communicate
	char buff[SIZE];
	int sent, received;
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("[-] Error %d: Cannot connect to server.\n", WSAGetLastError());
		return 0;
	}
	printf("[+] Connected to server.!\n");
	memset(&buff, 0, sizeof(buff));
	for (;;) {
		displayChoice();
		gets_s(buff, SIZE);
		printf("\n");
		switch (buff[0]) { // client's choice
		case '1':
			handleLogIn(client);
			break;
		case '2':
			handlePost(client);
			break;
		case '3':
			handleQuit(client);
			break;
		default:
			printf("%s", "Invalid choice!\n");
			break;
		}
	}

	closesocket(client);
	WSACleanup();
	return 0;
}

int Send(SOCKET sock, char* buf, int len, int flag) {
	int sent; // number of bytes have been sent
	int left; // number of bytes left
	left = len;
	while (left) {
		sent = send(sock, buf, left, flag);
		if (sent == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot send data to server !.\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		left -= sent;
		buf += sent;
	}
	return len;
}

int Receive(SOCKET sock, char* buf, int len, int flag) {
	int received; // number of bytes have been received
	int left; // number of bytes left
	left = len;
	while (left) {
		received = recv(sock, buf, left, flag);
		if (received == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot receive data from server !.\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		left -= received;
		buf += received;
	}
	return len;
}
