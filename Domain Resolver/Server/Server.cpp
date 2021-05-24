// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#include "ws2tcpip.h"

#pragma comment (lib, "Ws2_32.lib")
#define IP "127.0.0.1"
#define SIZE 2048

// Return hostname, input: IPv4 address
char* getHostName(char *buff) {
	char res[SIZE];
	sockaddr_in name;
	name.sin_family = AF_INET;
	inet_pton(AF_INET, buff, &name.sin_addr);
	name.sin_port = htons(27015);
	char hostname[SIZE], serverInfo[SIZE];
	DWORD ret;
	ret = getnameinfo((sockaddr *)&name, sizeof(sockaddr), hostname, NI_MAXHOST, serverInfo, NI_MAXSERV, NI_NUMERICSERV);

	if (ret != 0) {
		strcpy_s(res, SIZE, "[-] Error: Cannot get hostname!\n");
	}
	else {
		res[0] = '['; res[1] = '+'; res[2] = ']'; res[3] = ' ';
		strcpy_s(res + 4, SIZE - 4, hostname);
	}

	return res;
}

// Return: IP address, input: hostname
char* getAddr(char *buff) { 
	char res[SIZE];
	addrinfo *result; 
					  
	int rc;
	sockaddr_in *address;
	addrinfo hints; 
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	rc = getaddrinfo(buff, NULL, &hints, &result);

	// Get the address's info
	char ipStr[INET_ADDRSTRLEN];
	if (rc == 0) {
		address = (sockaddr_in *)result->ai_addr;
		inet_ntop(AF_INET, &address->sin_addr, ipStr, sizeof(ipStr));
		res[0] = '['; res[1] = '+'; res[2] = ']'; res[3] = ' ';
		strcpy_s(res + 4, SIZE - 4, ipStr);
	}
	else {
		printf("[-] Error %d: Bad request!\n", WSAGetLastError());
		strcpy_s(res, SIZE, "[-] Error: Bad request!");
	}

	freeaddrinfo(result);
	return res;
}

int main(int argc, char **argv)
{	
	if (argc != 2) {
		printf("usage: server.exe PortNumber\n");
		return 0;
	}
	int port = atoi(argv[1]);
	
	// Inittiate socket
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("[-] Winsock 2.2 is not supported!\n");
		return 0;
	}

	// Construct socket
	SOCKET server;
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

	// Communicate
	sockaddr_in clientAddr;
	char buff[SIZE], clientIP[INET_ADDRSTRLEN], msg[SIZE];
	int received, sent, clientAddrLen = sizeof(clientAddr), clientPort;

	for (;;) {
		received = recvfrom(server, buff, SIZE, 0, (sockaddr *)&clientAddr, &clientAddrLen);
		if (received == SOCKET_ERROR) {
			printf("[-] Error %d: Cannot received data from client!\n", WSAGetLastError());
		}
		else if (received > 0) {
			buff[received] = 0;
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP)); // convert IP addr from binary to text form
			clientPort = ntohs(clientAddr.sin_port);
			printf("[+] Received from client %s at port %d: %s\n", clientIP, clientPort, buff);

			// Check type of client's request (IPv4 or Hostname)
			in_addr addr;
			int isIPv4 = inet_pton(AF_INET, buff, &addr);

			if (isIPv4 == 1) strcpy_s(msg, SIZE, getHostName(buff));
			else strcpy_s(msg, SIZE, getAddr(buff));

			// Send to Client
			sent = sendto(server, msg, sizeof(msg), 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
			if (sent == SOCKET_ERROR) {
				printf("[-] Error %d: Cannot send message to client!\n", WSAGetLastError());
			}
		}
		
	}

	closesocket(server);
	WSACleanup();
    return 0;
}

