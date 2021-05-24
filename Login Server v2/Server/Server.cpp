// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "ws2tcpip.h"
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
fd_set readfds, initfds;
struct clientST {
	char clientIP[INET_ADDRSTRLEN];
	int clientPort;
	SOCKET sock;
	int status; // loged in or not
};


int Receive(SOCKET sock, char* buf, int len, int flag);
int Send(SOCKET sock, char* buf, int len, int flag);

void handleClient(struct clientST*);
const char* handleLogIn(struct clientST* client, char* buf);
const char* handlePost(struct clientST* client);
const char* handleQuit(struct clientST* client, int quitType);

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

	// Bind address to socket
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
	char buf[SIZE]; memset(buf, 0, SIZE);

	struct clientST clients[FD_SETSIZE]; // contains clients
	struct clientST new_client;

	for (int i = 0; i < FD_SETSIZE; i++) {
		clients[i].sock = 0;
		clients[i].status = 0;
	}
	
	FD_ZERO(&initfds);
	FD_SET(server, &initfds);

	char clientIP[INET_ADDRSTRLEN];
	sockaddr_in clientAddr;
	int clientPort, clientAddrLen = sizeof(clientAddr);
	SOCKET client;
	
	int nEvents;
	for (;;) {
		readfds = initfds;
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("[-] Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		// new client connection
		if (FD_ISSET(server, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((client = accept(server, (sockaddr*)&clientAddr, &clientAddrLen)) < 0) {
				printf("[-] Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				printf("[+] Connected to client %s\n", inet_ntoa(clientAddr.sin_addr));
				inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
				clientPort = ntohs(clientAddr.sin_port);

				/* assign value to client */
				struct clientST new_client;
				new_client.clientPort = clientPort;
				strcpy(new_client.clientIP, clientIP);
				new_client.sock = client;

				int i;
				for (i = 0; i < FD_SETSIZE; i++) {
					if (clients[i].sock == 0) {
						clients[i] = new_client;
						FD_SET(clients[i].sock, &initfds);
						break;
					}
				}
				if (i == FD_SETSIZE) {
					printf("FD_SETSIZE: %d\n", FD_SETSIZE);
					printf("[-] Too many clients!\n");
					closesocket(client);
				}
				if (--nEvents == 0) continue; // no events left
			}
		}

		// handle client's requests
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (clients[i].sock == 0) continue;
			if (FD_ISSET(clients[i].sock, &readfds)) {
				handleClient(&clients[i]); 
			}
			if (--nEvents <= 0) continue;
		}
	}
	closesocket(server);
	WSACleanup();
    return 0;
}


/* handle log in of client*/
void handleClient(struct clientST *client) {
	printf("Handling client %s: %d \n", client->clientIP, client->clientPort);
	char buf[SIZE];
	memset(buf, 0, SIZE);
	int received;
	char len_str[8]; // store len of incoming request in ascii form
	memset(len_str, 0, 8);
	received = recv(client->sock, len_str, 8, MSG_WAITALL); 
	if (received <= 0) { // client close program or hit ctrl + c
		int quitType = -1;
		handleQuit(client, quitType); return;
	}

	int len = atoi(len_str); // convert len to integer
	len = ntohs(len); // convert to host order

	received = Receive(client->sock, buf, len, 0); // receive len-bytes data
	char response[4]; // store response to send to client

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


/* Handle client log in with username contained in buf */
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
			if (tmp[strlen(username) + 1] == '0' ) { // and not blocked
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
	quitType = -1 indicates client close program or hit ctrl + c
	quitType = 1 indicates client choose quit mode from program */
const char* handleQuit(struct clientST* client, int quitType) {
	char response[4];
	if (quitType == -1) {
		FD_CLR(client->sock, &initfds);
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

/*Handle post from client */
const char* handlePost(struct clientST* client) {
	if (client->status == 1) return POST_OK; // if client already logged in
	return POST_FAIL;
}

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
