# tools/upload_custom.py
import os
import configparser
import urllib.request
from SCons.Script import Import

# Import the *global* PlatformIO env (PRE scripts operate on the global env)
Import("env")

print(">>> upload_custom.py loaded — PIOENV:", env.subst("$PIOENV"))

def download_and_prepare_upload(env):
    print("--- Custom Firmware Download & Upload Script ---")
    pio_env_name = env.subst("$PIOENV")
    project_dir = env.subst("$PROJECT_DIR")
    manifest_path = os.path.join(project_dir, "build", "manifest.ini")

    # strip _latest
    manifest_env_name = pio_env_name.replace("_latest", "")
    print(f"✓ Running for PIO environment '{pio_env_name}'.")
    print(f"  - Looking for manifest section: '{manifest_env_name}'")

    if not os.path.exists(manifest_path):
        print(f"✗ Error: Manifest file not found at '{manifest_path}'")
        env.Exit(1)

    config = configparser.ConfigParser()
    config.read(manifest_path)

    if not config.has_section(manifest_env_name):
        print(f"✗ Error: No section for '{manifest_env_name}' found in manifest.ini")
        env.Exit(1)

    try:
        filename = config.get(manifest_env_name, 'filename')
        version_tag = config.get(manifest_env_name, 'version_string')
    except configparser.NoOptionError as e:
        print(f"✗ Error: Missing required option in manifest.ini: {e}")
        env.Exit(1)

    print(f"✓ Found manifest entry: version={version_tag}, file={filename}")

    github_repo = "jmamma/MCL"
    url = f"https://github.com/{github_repo}/releases/download/{version_tag}/{filename}"

    build_dir = env.subst("$BUILD_DIR")
    os.makedirs(build_dir, exist_ok=True)
    download_path = os.path.join(build_dir, "firmware.hex")

    print(f"→ Downloading firmware from: {url}")
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'PlatformIO-PreUpload-Script'})
        with urllib.request.urlopen(req) as response, open(download_path, 'wb') as out_file:
            if getattr(response, "status", 200) != 200:
                print(f"✗ Error: Download failed, status {getattr(response,'status', 'n/a')}")
                env.Exit(1)
            out_file.write(response.read())
        print(f"✓ Saved to '{os.path.relpath(download_path, project_dir)}'")
    except urllib.error.HTTPError as e:
        print(f"✗ HTTP Error {e.code}: {e.reason}")
        env.Exit(1)
    except urllib.error.URLError as e:
        print(f"✗ URL Error: {e.reason}")
        env.Exit(1)
    except Exception as e:
        print(f"✗ Unexpected download error: {e}")
        env.Exit(1)

    # Replace upload hex command for avr (avrdude)
    env.Replace(UPLOADCMD=f'"$UPLOADER" $UPLOADERFLAGS -U flash:w:"{download_path}":i')
    print(f"✓ Upload command configured to use '{filename}'")
    print("------------------------------------------")


if env.subst("$PIOENV").endswith("_latest"):
    print("✓ Forcing nobuild for upload")
    download_and_prepare_upload(env)
else:
    print("✓ upload_custom.py: not a *_latest env — no changes applied.")

