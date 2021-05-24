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

char username[SIZE];

bool logedIn;

void displayChoice() {
	printf("---------------------------------\n");
	if (!logedIn) printf("%s", "1. Log in\n");
	printf("%s", "2. Post message\n");
	printf("%s", "3. Quit\n");
	printf("%s", "Enter your choice: ");
}

void handleLogIn(SOCKET client) {
	char buf[SIZE];
	char responseFromServer[4];
	printf("Username: ");
	//char username[SIZE];
	gets_s(username, SIZE);
	printf("\n");
	strcpy(buf, username);
	strcat(buf, "01"); // indicate client choose Log In mode
	buf[strlen(buf)] = 0;
	int sent = send(client, buf, strlen(buf), 0);
	if (sent == SOCKET_ERROR) {
		printf("[-] Error %d: Cannot send data to server !.\n", WSAGetLastError());
		return;
	}
	int received = recv(client, responseFromServer, 4, 0);
	if (strncmp(responseFromServer, LOG_OK, 2) == 0) {
		logedIn = true;
		printf("Successfully loged in!\n");
	}
	else {
		printf("Failed to log in! Please try again!\n");
	}
}

void handlePost(SOCKET client) {
	char buf[SIZE];
	char responseFromServer[4];
	memset(buf, 0, SIZE);
	int sent, received;
	printf("Message: ");
	gets_s(buf, SIZE);
	strcat(buf, "02"); // indicate user choose Post message mode
	printf("\n");
	sent = send(client, buf, strlen(buf), 0);
	if (sent == SOCKET_ERROR) {
		printf("[-] Error %d: Cannot send data to server !.\n", WSAGetLastError());
		return;
	}
	received = recv(client, responseFromServer, 4, 0);
	if (strncmp(responseFromServer, POST_OK, 2) == 0) {
		printf("Successfully post message!\n");
	}
	else {
		printf("Failed to post message! Please try again!\n");
	}
}

void handleQuit(SOCKET client) {
	char buf[SIZE];
	strcpy(buf, username);
	strcat(buf, "03");
	char responseFromServer[4];
	int sent = send(client, buf, strlen(buf), 0);
	if (sent == SOCKET_ERROR) {
		printf("[-] Error %d: Cannot send data to server !.\n", WSAGetLastError());
		return;
	}
	int received = recv(client, responseFromServer, 4, 0);
	if (strncmp(responseFromServer, QUIT_OK, 2) == 0) {
		printf("Quited!\n");
		logedIn = false;
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
		switch (buff[0]) {
		case '1':
			if (!logedIn) {
				handleLogIn(client);
			}
			else {
				printf("You are already loged in!\n");
				printf("Please quit to log in with another account!\n");
			}
			break;
		case '2':
			if (!logedIn) {
				printf("You need to log in first!\n");
			}
			else {
				handlePost(client);
			}
			break;
		case '3':
			if (!logedIn) {
				printf("You are not loged in!\n");
			}
			else {
				handleQuit(client);
			}
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