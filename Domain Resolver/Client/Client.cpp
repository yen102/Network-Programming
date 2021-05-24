// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "ws2tcpip.h"

#pragma comment (lib, "Ws2_32.lib")
#define SIZE 2048


int main(int argc, char **argv)
{	
	// Get input
	if (argc != 3) {
		printf("usage: client.exe ServerIPAddress ServerPortNumber\n");
		return 0;
	}
	int serverPort = atoi(argv[2]);
	char* serverIP = argv[1];

	// Inittiate socket
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("[-] Winsock 2.2 is not supported!\n");
		return 0;
	}

	// Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client == INVALID_SOCKET) {
		printf("[-] Error %d: Cannot create client socket!\n", WSAGetLastError());
		return 0;
	}
	printf("[+] Successfully created client socket!\n");

	// Server socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);
	
	// Communicate
	char buff[SIZE];
	int sent, received, serverAddrLen = sizeof(serverAddr), msgLen;

	for(;;) {
		// Send Request
		printf("Send to Server: ");
		fgets(buff, SIZE, stdin);
		msgLen = strlen(buff)-1; // exclude "/n" character
		if (msgLen <= 0) break;
		sent = sendto(client, buff, msgLen, 0, (sockaddr *)&serverAddr, serverAddrLen);
		if (sent == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot send request to server!\n", WSAGetLastError());
		}

		// Receive Response
		received = recvfrom(client, buff, SIZE, 0, (sockaddr *)&serverAddr, &serverAddrLen);
		if (received == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot receive response from server!\n", WSAGetLastError());
		}
		if (received > 0) {
			printf("[+] Received from Server: \n%s\n\n", buff);
		}
		
	}

	closesocket(client);
	WSACleanup();
	return 0;
}