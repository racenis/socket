
win32:
	g++ service.cpp -o service.exe -std=c++20 -lws2_32
	
win32_cli:
	g++ cli.cpp -o cli.exe -std=c++20 -lws2_32