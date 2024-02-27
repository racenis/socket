
default: service cli

service:
	g++ service.cpp -o service -std=c++20

cli:
	g++ cli.cpp -o cli -std=c++20
	
win32:
	g++ service.cpp -o service.exe -std=c++20 -lws2_32
	
win32_cli:
	g++ cli.cpp -o cli.exe -std=c++20 -lws2_32
