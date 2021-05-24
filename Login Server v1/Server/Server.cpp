// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "process.h"
#include "time.h"

#define IP "127.0.0.1"
#define SIZE 2048

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
	SOCKET client;
	char username[SIZE];
};

/* handle log in request, with buff contain username and footer indicate log in mode
return result code */
const char* handleLogIn(char* buff) {
	int size = strlen(buff) - 2; // size of username
	printf("buff: %s\n", buff);
	FILE* fp = fopen("account.txt", "r+");
	if (fp == NULL) {
		printf("[-] Cannot open file!\n");
	}
	long bytes; // count number of bytes from beginning of the file till current position
	char tmp[SIZE];
	while (fgets(tmp, SIZE, fp)) {
		bytes = ftell(fp);
		if (strncmp(buff, tmp, size) == 0) { // if found matched username in account file
			if (tmp[size + 1] == '0') { // and current status is not loged in
				tmp[size + 1] = '1'; // update user's status
				fseek(fp, bytes - strlen(tmp) - 1, SEEK_SET);
				fputs(tmp, fp);
				fclose(fp);
				return LOG_OK;
			}
		}
	}
	fclose(fp);
	return LOG_FAIL;
}

// handle Post message request
// return result code
const char* handlePost(char* buff) {
	return POST_OK;
}

/* handle quit request, with buff contains username and footer indicate log in mode
return result code */
const char* handleQuit(char* buff) {
	int size = strlen(buff) - 2;
	FILE* fp = fopen("account.txt", "r+");
	if (fp == NULL) {
		printf("[-] Cannot open file!\n");
	}
	long bytes; // count number of bytes from beginning of the file till current position
	char tmp[SIZE];
	while (fgets(tmp, SIZE, fp)) {
		bytes = ftell(fp);
		if (strncmp(buff, tmp, size) == 0) { // if found matched username in account file
			if (tmp[size + 1] == '1') {
				tmp[size + 1] = '0'; // update user's status
				fseek(fp, bytes - strlen(tmp) - 1, SEEK_SET); // set file pointer to the beginning of current line
				fputs(tmp, fp); // overwrite current line
				fclose(fp);
				return QUIT_OK;
			}
		}
	}
	fclose(fp);
	return QUIT_FAIL;
}

unsigned __stdcall handleClient(void *param) {
	char buff[SIZE];
	struct clientST* clientPrt = (struct clientST*)param;
	SOCKET client = clientPrt->client;
	int clientPort = clientPrt->clientPort;
	char clientIP[INET_ADDRSTRLEN];
	strcpy(clientIP, clientPrt->clientIP);
	char username[SIZE];
	memset(username, SIZE, 0);

	int received, sent;
	for (;;) {
		memset(buff, 0, SIZE);
		received = recv(client, buff, SIZE, 0);
		if (received < 0) { // user closed program or hit ctrl + c
			printf("[-] Error %d: Cannot received request from client.\n", WSAGetLastError());
			if (username[0]) { // if there's an user has loged in
				strcat(username, "03"); // quit
				printf("username: %s\n", username);
				handleQuit(username);
			}
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
			char response[4];
			FILE* fp = fopen("log_20183862.txt", "a+");
			if (fp == NULL) {
				printf("[-] Cannot open log file!\n");
			}
			char timeBuf[128]; // contain time information
			memset(timeBuf, 0, 128);
			struct tm *t;
			time_t now;
			switch (buff[received - 1]) {
			case '1':
				strncpy(username, buff, received - 2);
				username[received - 2] = 0;
				strcpy(response, handleLogIn(buff));
				sent = send(client, response, 4, 0);
				break;
			case '2':
				strcpy(response, handlePost(buff));
				sent = send(client, response, 4, 0);
				break;
			case '3':
				strcpy(response, handleQuit(buff));
				sent = send(client, response, 4, 0);
				break;
			default:
				printf("[-] Some errors have orcured during receiving client's requests!\n");
				break;
			}
			/* write log to file */
			time(&now);
			t = localtime(&now);
			strftime(timeBuf, 128, "%d/%m/%Y %X", t);
			fputs(clientIP, fp);
			fputs(":", fp);
			fprintf(fp, "%d", clientPort);
			fputs(" [", fp);
			fputs(timeBuf, fp);
			fputs("] $ ", fp);
			fputs(buff, fp);
			fputs(" $ ", fp);
			fputs(response, fp);
			fputs("\r\n", fp);
			fclose(fp);
		}
	}
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
	char clientIP[INET_ADDRSTRLEN];
	sockaddr_in clientAddr;
	int clientPort, clientAddrLen = sizeof(clientAddr);

	SOCKET client;
	for (;;) {
		printf("\n");
		client = accept(server, (sockaddr *)&clientAddr, &clientAddrLen);
		if (client == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot connect to client.!\n", WSAGetLastError());
			return 0;
		}

		printf("[+] A connection has been established.\n");
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
		clientPort = ntohs(clientAddr.sin_port);
		printf("[+] Accept incoming request from client %s:%d\n", clientIP, clientPort);
		struct clientST new_client;
		new_client.clientPort = clientPort;
		strcpy(new_client.clientIP, clientIP);
		new_client.client = client;

		struct clientST* clientPrt = &new_client;
		_beginthreadex(0, 0, handleClient, (void *)clientPrt, 0, 0); //start thread

	}
	closesocket(server);
	closesocket(client);
	WSACleanup();

	return 0;
}

