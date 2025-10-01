# tools/upload_custom.py
import os
import configparser
import urllib.request
import tempfile
from SCons.Script import Import

# Import the *global* PlatformIO env (PRE scripts operate on the global env)
Import("env")

print(">>> upload_custom.py loaded — PIOENV:", env.subst("$PIOENV"))

def download_manifest(repo, branch="main"):
    """
    Downloads the manifest.ini file from the specified GitHub repository to a temporary file.
    """
    manifest_url = f"https://raw.githubusercontent.com/{repo}/{branch}/manifest.ini"
    print(f"→ Downloading latest manifest from: {manifest_url}")

    try:
        # Create a temporary file to store the manifest
        temp_manifest = tempfile.NamedTemporaryFile(delete=False, suffix=".ini")
        manifest_path = temp_manifest.name
        temp_manifest.close() # Close the file so it can be opened again on Windows

        req = urllib.request.Request(manifest_url, headers={'User-Agent': 'PlatformIO-PreUpload-Script'})
        with urllib.request.urlopen(req) as response, open(manifest_path, 'wb') as out_file:
            if getattr(response, "status", 200) != 200:
                print(f"✗ Error: Manifest download failed, status {getattr(response,'status', 'n/a')}")
                env.Exit(1)
            out_file.write(response.read())
        
        print(f"✓ Manifest saved to temporary location: '{manifest_path}'")
        return manifest_path

    except urllib.error.HTTPError as e:
        print(f"✗ HTTP Error {e.code}: {e.reason} while downloading manifest.")
        env.Exit(1)
    except urllib.error.URLError as e:
        print(f"✗ URL Error: {e.reason} while downloading manifest.")
        env.Exit(1)
    except Exception as e:
        print(f"✗ Unexpected error downloading manifest: {e}")
        env.Exit(1)


def download_and_prepare_upload(env):
    print("--- Custom Firmware Download & Upload Script ---")
    pio_env_name = env.subst("$PIOENV")
    project_dir = env.subst("$PROJECT_DIR")
    github_repo = "jmamma/MCL"

    # Download the latest manifest.ini from the remote repository
    manifest_path = download_manifest(github_repo)

    # strip _latest
    manifest_env_name = pio_env_name.replace("_latest", "")
    print(f"✓ Running for PIO environment '{pio_env_name}'.")
    print(f"  - Looking for manifest section: '{manifest_env_name}'")

    if not os.path.exists(manifest_path):
        print(f"✗ Error: Temporary manifest file not found at '{manifest_path}'")
        env.Exit(1)

    config = configparser.ConfigParser()
    config.read(manifest_path)

    # Clean up the temporary manifest file after reading it
    os.remove(manifest_path)
    print(f"✓ Temporary manifest file cleaned up.")

    if not config.has_section(manifest_env_name):
        print(f"✗ Error: No section for '{manifest_env_name}' found in the downloaded manifest.ini")
        env.Exit(1)

    try:
        filename = config.get(manifest_env_name, 'filename')
        version_tag = config.get(manifest_env_name, 'version_string')
    except configparser.NoOptionError as e:
        print(f"✗ Error: Missing required option in manifest.ini: {e}")
        env.Exit(1)

    print(f"✓ Found manifest entry: version={version_tag}, file={filename}")

    
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
