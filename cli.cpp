#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <winsock2.h>
	
	#define ERROR_VALUE WSAGetLastError()
	#define close(X) closesocket(X)
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h> 
	
	#define ERROR_VALUE errno
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
#endif

int main(int argc, const char** argv) {
	int error;
#ifdef _WIN32
	WSADATA wsa_data;
	error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (error != 0) {
		printf("Winsock startup failed: %i\n", error);
		abort();
	}
#endif
	
	printf("Connecting...\n");
	
	sockaddr_in socket_info;
	int connect_socket;
	
	memset(&socket_info, 0, sizeof(socket_info));
	
	socket_info.sin_family = AF_INET;
	socket_info.sin_port = htons(4444);
	socket_info.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect_socket == INVALID_SOCKET) {
		printf("Error opening service socket: %i\n", ERROR_VALUE);
		abort();
	}
	
	error = connect(connect_socket, (sockaddr*)&socket_info, sizeof(socket_info));
    if (error == SOCKET_ERROR) {
        printf("Error connecting to service: %i\n", ERROR_VALUE);
        abort();
    }
	
	char msg[1000];
	
	recv(connect_socket, msg, 1000, 0);
	
	printf("%s\n", msg);
	
	return 0;
}