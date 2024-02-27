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

enum DatagramType {
	DATAGRAM_BROADCAST,
	DATAGRAM_REPLY
};

struct Datagram {
	DatagramType type;
	uint8_t mac_address[6];
};

static sockaddr_in outgoing_broadcast_info;
static sockaddr_in incoming_broadcast_info;
static int broadcast_socket = 0;

static sockaddr_in server_info;
static int server_socket = 0;


// saraksts ar atrastajiem kaimiņiem
struct Neighbor {
	uint8_t mac_address[6];
	uint32_t ip_address;
	
	uint32_t last_seen;
};

static Neighbor neighbors[100];
size_t neighbor_count = 0;



void InitNetwork(bool skipbind = false) {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		abort();
	}
	
	// atrašanas servisa socketi
	memset(&outgoing_broadcast_info, 0, sizeof(outgoing_broadcast_info));
	memset(&incoming_broadcast_info, 0, sizeof(incoming_broadcast_info));
	
	outgoing_broadcast_info.sin_family = AF_INET;
	outgoing_broadcast_info.sin_port = htons(4444);
	outgoing_broadcast_info.sin_addr.s_addr = inet_addr("255.255.255.255");
	
	incoming_broadcast_info.sin_family = AF_INET;
	incoming_broadcast_info.sin_port = htons(4444);
	incoming_broadcast_info.sin_addr.s_addr = htonl(INADDR_ANY);
	
	
	// broadcast spcket
	broadcast_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if (broadcast_socket == INVALID_SOCKET) {
		printf("Error opening outbound socket: %i\n", WSAGetLastError());
		abort();
	}
	
	char broadcast = 1;
	setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	
	if (skipbind) return;
	
	iResult = bind(broadcast_socket, (SOCKADDR *) &incoming_broadcast_info, sizeof(incoming_broadcast_info));
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error %u\n", WSAGetLastError());
        closesocket(broadcast_socket);
        WSACleanup();
        abort();
    }
	
	
	
	// tcp socket
	memset(&server_info, 0, sizeof(server_info));
	
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(4444);
	server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		printf("Error opening server socket: %i\n", WSAGetLastError());
		abort();
	}
	
	iResult = bind(server_socket, (SOCKADDR *) &server_info, sizeof(server_info));
    if (iResult == SOCKET_ERROR) {
        printf("bind tcvpfailed with error %u\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        abort();
    }
}

void UninitNetwork() {
	int iResult = closesocket(broadcast_socket);
    if (iResult == SOCKET_ERROR) {
        printf("closesocket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        abort();
    }
	
	WSACleanup();
}

void GetSelfMAC(uint8_t* mac) {
	static uint8_t self_mac[6] = {(uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand()};
	memcpy(mac, self_mac, 6);
}

uint32_t GetTime() {
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void BroadcastMessage(const char* msg, int len) {
	int iResult = sendto(broadcast_socket,
                     msg, len, 0, (SOCKADDR *) & outgoing_broadcast_info, sizeof (outgoing_broadcast_info));
    if (iResult == SOCKET_ERROR) {
        printf("sendto failed with error: %d\n", WSAGetLastError());
		WSACleanup();
       abort();
    }
	
	printf("SENT: %d\n", iResult);
}

void ReceiveMessage(char* msg, size_t max_len, uint32_t* sender_address = nullptr) {
	struct sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof (SenderAddr);
	
	
	int blen = recvfrom(broadcast_socket, msg, max_len, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
    if (blen == -1) {
		printf("RECIVE failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		abort();
	}
	
	
	if (sender_address) *sender_address = SenderAddr.sin_addr.S_un.S_addr;
}

void ProcessServer() {
	listen(server_socket, 1);
	int connection = accept(server_socket, nullptr, nullptr);
	
	char msg[100] = "HELLO FROM SERVER!";
	
	send(connection, msg, sizeof(msg), 0);
	closesocket(connection);
}

int main(int argc, const char** argv) {
	neighbors[0] = {
		{100, 134, 151, 01, 149, 24},
		0x8304861F,
		GetTime()
	};
	
	neighbors[2] = {
		{100, 134, 151, 01, 149, 24},
		0xF004811F,
		GetTime()
	};
	
	neighbors[1] = {
		{100, 41, 151, 61, 149, 71},
		0x83E486AF,
		GetTime()
	};
	neighbor_count = 3;
	
	
	printf("ASDNASFABNFAIF\n");
	
	char msg[100] = "I am LIGMA MAN!";
	
	if (argc > 1) {
		InitNetwork(true);
		
		printf("broadcasting\n");
		char msg[100];
		unsigned char mac[6];
		GetSelfMAC(mac);
		sprintf(msg, "%s %02x:%02x:%02x:%02x:%02x:%02x\n", argv[1], mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		BroadcastMessage(msg, 100);
	} else {
		InitNetwork();
		
		std::thread broadcast_server([&](){
			for (;;) {
				printf("waiting\n");
				char rec[1000];
				uint32_t addr;
				ReceiveMessage(rec, 100, &addr);
				
				unsigned char poop[4];
				*(uint32_t*)poop = addr;
				
				printf("message: %s \n", rec);
				printf("from: %i %i %i %i \n", poop[0], poop[1], poop[2], poop[3]);
			}
		});
		
		std::thread broadcast_client([&](){
			for (;;) {
				printf("*BROADCASTING*\n");
				std::this_thread::sleep_for(std::chrono::seconds(30));
			}
		});
		
		for (;;) {
			//std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			ProcessServer();
		}
	}
	
	
	
	
	UninitNetwork();
	return 0;
}