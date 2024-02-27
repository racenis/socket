#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <chrono>
#include <thread>

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

// UDP paketes saturs
enum DatagramType {
	DATAGRAM_BROADCAST,
	DATAGRAM_REPLY
};

struct Datagram {
	DatagramType type;
	uint8_t mac_address[6];
};

// UDP tīkla servisa socketi
static sockaddr_in outgoing_broadcast_info;
static sockaddr_in incoming_broadcast_info;
static int broadcast_socket = 0;

// TCP servera socketi
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


// atver socketus
void InitNetwork() {
	int error;
#ifdef _WIN32
	WSADATA wsa_data;
	error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (error != 0) {
		printf("Winsock startup failed: %i\n", error);
		abort();
	}
#endif
	
	// UDP tīkla servisa socketi
	memset(&outgoing_broadcast_info, 0, sizeof(outgoing_broadcast_info));
	memset(&incoming_broadcast_info, 0, sizeof(incoming_broadcast_info));
	
	outgoing_broadcast_info.sin_family = AF_INET;
	outgoing_broadcast_info.sin_port = htons(4444);
	outgoing_broadcast_info.sin_addr.s_addr = inet_addr("255.255.255.255"); // īpašā adrese
	
	incoming_broadcast_info.sin_family = AF_INET;
	incoming_broadcast_info.sin_port = htons(4444);
	incoming_broadcast_info.sin_addr.s_addr = htonl(INADDR_ANY);
	
	broadcast_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if (broadcast_socket == INVALID_SOCKET) {
		printf("Error opening broadcast socket: %i\n", ERROR_VALUE);
		abort();
	}
	
	char broadcast = 1;
	setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	
	error = bind(broadcast_socket, (sockaddr*)&incoming_broadcast_info, sizeof(incoming_broadcast_info));
    if (error == SOCKET_ERROR) {
        printf("Error binding broadcast socket: %i\n", ERROR_VALUE);
        abort();
    }
	
	
	
	// TCP servera socketi
	memset(&server_info, 0, sizeof(server_info));
	
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(4444);
	server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		printf("Error opening server socket: %i\n", ERROR_VALUE);
		abort();
	}
	
	error = bind(server_socket, (sockaddr*)&server_info, sizeof(server_info));
    if (error == SOCKET_ERROR) {
        printf("Error binding server socket: %i\n", ERROR_VALUE);
        abort();
    }
}

// aizver socketus
void UninitNetwork() {
	int error = close(broadcast_socket);
    if (error == SOCKET_ERROR) {
        printf("Broadcast socket closing failed: %i\n", ERROR_VALUE);
        abort();
    }
	
	error = close(server_socket);
    if (error == SOCKET_ERROR) {
        printf("Server socket closing failed: %i\n", ERROR_VALUE);
        abort();
    }

#ifdef _WIN32
	WSACleanup();
#endif
}

// atrod savu MAC adresi
void GetSelfMAC(uint8_t* mac) {
	// MAC adresi var atrast ar dažādiem sistēmas izsaukiem, bet man šodien nav
	// garastāvokļa ar to ņemties
	srand(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	static uint8_t self_mac[6] = {(uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand(), (uint8_t)rand()};
	memcpy(mac, self_mac, 6);
}

// atrod laiku, sekundēs no laika sākuma
uint32_t GetTime() {
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// nosūta UDP paziņojumu visiem tīklā pieslēgtajiem datoriem
void BroadcastMessage(const char* msg, size_t len) {
	int error = sendto(broadcast_socket, msg, len, 0, (sockaddr*)&outgoing_broadcast_info, sizeof(outgoing_broadcast_info));
	if (error == SOCKET_ERROR) {
		printf("BroadcastMessage failed: %d\n", ERROR_VALUE);
		abort();
	}
}

// nosūta UDP paziņojumu specifiskam datoram
void SendMessage(uint32_t address, const char* msg, size_t len) {
	sockaddr_in message_info;
	memset(&message_info, 0, sizeof(message_info));

	message_info.sin_family = AF_INET;
	message_info.sin_port = htons(4444);
	message_info.sin_addr.S_un.S_addr = address;

	int error = sendto(broadcast_socket, msg, len, 0, (sockaddr*)&message_info, sizeof(message_info));
	if (error == SOCKET_ERROR) {
		printf("SendMessage failed: %d\n", ERROR_VALUE);
		abort();
	}
}

// saņem UDP paziņojumu
void ReceiveMessage(char* msg, size_t max_len, uint32_t* sender_address = nullptr) {
	sockaddr_in address;
    int address_size = sizeof(address);
	
	int error = recvfrom(broadcast_socket, msg, max_len, 0, (sockaddr*)&address, &address_size);
    if (error == SOCKET_ERROR) {
		printf("ReceiveMessage failed: %d\n", ERROR_VALUE);
		abort();
	}
	
	if (sender_address) *sender_address = address.sin_addr.S_un.S_addr;
}

// apstrādā TCP serveri
void ProcessServer() {
	
	// pieņem jaunu savienojumu
	listen(server_socket, 1);
	int connection = accept(server_socket, nullptr, nullptr);
	
	// sagatavo atbildi
	char msg[1000] = "MAC Address \t\tIP Address \tTime since last seen\n";
	
	for (size_t i = 0; i < neighbor_count; i++) {
		uint32_t time_since_last_seen = GetTime() - neighbors[i].last_seen;
	
		// izlaiž tos datorus, kuri nav atsaukušies ilgāk par 30 sekundēm
		if (time_since_last_seen > 30) continue;
		
		char entry[100];
		
		// sadala IP adresi pa baitiem
		uint8_t ip_address[4];
		*(uint32_t*)&ip_address = neighbors[i].ip_address;
		
		sprintf(entry, 
			"%02x:%02x:%02x:%02x:%02x:%02x\t%i.%i.%i.%i\t%i\n",
			neighbors[i].mac_address[0], neighbors[i].mac_address[1],
			neighbors[i].mac_address[2], neighbors[i].mac_address[3],
			neighbors[i].mac_address[4], neighbors[i].mac_address[5],
			ip_address[0], ip_address[1], ip_address[2], ip_address[3],
			time_since_last_seen
		);
		
		strcat(msg, entry);
	}
	
	// nosūta atbildi un aizver savienojumu
	send(connection, msg, sizeof(msg), 0);
	close(connection);
	
	printf("Sent server response!\n");
}

int main(int argc, const char** argv) {
	InitNetwork();
	
	std::thread broadcast_server([&](){
		for (;;) {
			printf("Waiting for incoming messages...\n");
			
			Datagram received_datagram;
			uint32_t sender_address;
			ReceiveMessage((char*)&received_datagram, sizeof(received_datagram), &sender_address);
			
			switch (received_datagram.type) {
				case DATAGRAM_BROADCAST: {
					printf("Received broadcast!\n");
				
					Datagram reply;
					reply.type = DATAGRAM_REPLY;
					GetSelfMAC(reply.mac_address);
					
					SendMessage(sender_address, (char*)&reply, sizeof(reply));
					
					printf("Responded to broadcast!\n");
					} break;
				case DATAGRAM_REPLY: {
					printf("Received reply!\n");
					
					size_t neighbor = -1;
					for (size_t i = 0; i < neighbor_count; i++) {
						if (memcmp(received_datagram.mac_address, neighbors[i].mac_address, 6) == 0) {
							neighbor = i;
							break;
						}
					}
					
					if (neighbor == -1) {
						neighbors[neighbor_count].ip_address = sender_address;
						neighbors[neighbor_count].last_seen = GetTime();
						memcpy(neighbors[neighbor_count].mac_address, received_datagram.mac_address, 6);
						
						neighbor_count++;
					} else {
						neighbors[neighbor].last_seen = GetTime();
					}
					
					} break;
				default:
					printf("Unrecognized datagram type: %i\n", received_datagram.type);
			}
		}
	});
	
	std::thread broadcast_client([&](){
		for (;;) {
			Datagram broadcast;
			broadcast.type = DATAGRAM_BROADCAST;
			GetSelfMAC(broadcast.mac_address);
			
			BroadcastMessage((char*)&broadcast, sizeof(broadcast));
			
			printf("Broadcasted!\n");
			
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
	});
	
	for (;;) {
		ProcessServer();
	}
	
	UninitNetwork();
	
	return 0;
}