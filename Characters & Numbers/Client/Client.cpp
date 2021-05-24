// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#define SIZE 2048

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

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
	int msgLen;
	for (;;) {
		memset(&buff, 0, SIZE);
		printf("Send to server: ");
		//scanf("%[^\n]%*c", buff);
		//msgLen = strlen(buff);
		//printf("len: %d\n", msgLen);
		//if (msgLen > SIZE) {
		//	printf("[!] Warning: You can only send upto %d bytes data.\n", SIZE-1);
		//}
		gets_s(buff, SIZE);
		msgLen = strlen(buff);
		if (msgLen > SIZE) memset(buff + SIZE, 0, msgLen - SIZE);
		msgLen = min(msgLen, SIZE - 1);
		if (msgLen == 0) break;
		sent = send(client, buff, msgLen, 0);
		if (sent == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot send data to server !.\n", WSAGetLastError());
			break;
		}
		received = recv(client, buff, SIZE, 0);
		if (received == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot received data from server!\n", WSAGetLastError());
			break;
		}
		else if (received > 0) {
			buff[received] = 0;
			printf("Result received from server:\n");
			if (buff[0] == '-') { // request contains non-digit and non-alpha characters
				printf("Error!\n");
			}
			else {
				printf("%s\n", buff);
			}
		}	
	}

	closesocket(client);
	WSACleanup();
	return 0;
}