#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <chrono>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>


int main(int argc, const char** argv) {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		abort();
	}
	
	printf("Connecting...\n");
	
	sockaddr_in socket_info;
	int connect_socket = 0;
	
	memset(&socket_info, 0, sizeof(socket_info));
	
	socket_info.sin_family = AF_INET;
	socket_info.sin_port = htons(4444);
	socket_info.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect_socket == INVALID_SOCKET) {
		printf("Error opening server socket: %i\n", WSAGetLastError());
		abort();
	}
	
	iResult = connect(connect_socket, (SOCKADDR *) &socket_info, sizeof(socket_info));
    if (iResult == SOCKET_ERROR) {
        printf("connect tcvpfailed with error %u\n", WSAGetLastError());
        closesocket(connect_socket);
        WSACleanup();
        abort();
    }
	
	char msg[1000];
	
	recv(connect_socket, msg, 1000, 0);
	
	printf("%s\n", msg);
}