
win32:
	g++ main.cpp -o poo.exe -std=c++20 -lws2_32
	
win32_cli:
	g++ cli.cpp -o pee.exe -std=c++20 -lws2_32