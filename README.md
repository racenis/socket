# Take-home assignment for MikroTik

Compiles with GCC on Linux and with MinGW on Windows. Since Unix sockets and Winsock have basically the same API (Winsock has different startup/shutdown functions) I could use the same code for both OS's.

Use `make service && make cli` on Linux and `make win32 && make win32_cli` on Windows to compile.

When the `cli` program is run it will ping all running instances of `service` connected to the local network and report it back to the user.

Idk, I have no idea how networking works. 
