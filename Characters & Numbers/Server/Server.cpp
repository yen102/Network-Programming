// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "ws2tcpip.h"

#define IP "127.0.0.1"
#define SIZE 2048

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

char res[SIZE]; // stored handled string


/*
Input: a string
Return - if there's a non-digit and non-alpha character found, else return a string with alpha characters and digit characters seperated
*/

char* splitString(char *str) {
	memset(res, 0, SIZE);
	int len = strlen(str);
	char digit[SIZE]; // contains digit characters
	int i = 0;
	int j = 0;
	for (int s = 0; s < len; s++) {
		if (isdigit(str[s])) {
			digit[i] = str[s];
			i += 1;
		}
		else if (isalpha(str[s])) {
			res[j] = str[s];
			j += 1;
		}
		else {
			res[0] = '-';
			return res;
		}
	}
	digit[i] = 0;
	if (j != 0) {
		res[j] = 10; // new line
		strcpy_s(res + j + 1, SIZE - j - 1, digit); // append digit part to alpha part if there's alpha characters in input string
	}
	else {
		strcpy(res, digit);	// if there's only digit characters found
	}
	return res;
}


int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("usage: server.exe PortNumber\n");
		return 0;
	}
	int port = atoi(argv[1]);

	// Initiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("[-] Winsock 2.2 is not supported!\n");
		return 0;
	}

	// Construct socket
	SOCKET server;
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == INVALID_SOCKET) {
		printf("[-] Error %d: Cannot create server socket!\n", WSAGetLastError());
		return 0;
	}
	printf("[+] Successfully created server socket!\n");

	//Bind	address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, IP, &serverAddr.sin_addr);
	if (bind(server, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("[-] Error %d: Cannot bind address to socket!\n", WSAGetLastError());
		return 0;
	}
	printf("[+] Successfully binded address to server.\n");

	if (listen(server, 10)) {
		printf("[-] Error %d: Error in listening !.\n", WSAGetLastError());
		return 0;
	}
	printf("[+] Listening...\n");

	// Communicate
	char buff[SIZE], clientIP[INET_ADDRSTRLEN];
	sockaddr_in clientAddr;
	int received, sent, clientPort, clientAddrLen = sizeof(clientAddr);

	SOCKET client;
	for (;;) {
		printf("%s", "\n");
		client = accept(server, (sockaddr *)&clientAddr, &clientAddrLen);
		if (client == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot connect to client.!\n", WSAGetLastError());
			return 0;
		}

		printf("[+] A connection has been established.\n");
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
		clientPort = ntohs(clientAddr.sin_port);
		printf("[+] Accept incoming request from client %s:%d\n", clientIP, clientPort);

		for (;;) {
			received = recv(client, buff, SIZE, 0);
			if (received < 0) {
				printf("[-] Error %d: Cannot received data from client.\n", WSAGetLastError());
				closesocket(client);
				break;
			}
			else if (received == 0) {
				printf("[-] Client quited.\n");
				closesocket(client);
				break;
			}
			else {
				buff[received] = 0;
				printf("[+] Received from client %s:%d : %s\n", clientIP, clientPort, buff);
				char* splitedString; 
				splitedString = splitString(buff);
				memset(buff, 0, SIZE);
				strcpy(buff, splitedString);
				sent = send(client, buff, strlen(buff), 0);
				if (sent == SOCKET_ERROR || sent == 0) {
					printf("[-] Error: Cannot send data to client!\n");
					break;
				}
			}
		}
	}
	closesocket(server);
	closesocket(client);
	WSACleanup();

	return 0;
}

