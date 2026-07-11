import hashlib
import json
import os
import re
import subprocess
import time

import serial
import serial.tools.list_ports

Import("env")

# OTA deploy destination (Home Assistant with Terminal & SSH addon, key-based auth)
OTA_HOST = "root@192.168.1.6"
OTA_PORT = "22"
OTA_REMOTE_DIR = "/config/www/higrow"

def reset_esp32(source, target, env):
    """PlatformIO custom target to reset ESP32 via DTR"""

    # Find serial port
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("ERROR: No serial ports found!")
        return

    port = ports[0].device
    print(f"Resetting ESP32 on {port}...")

    try:
        s = serial.Serial(port, 115200, timeout=1)
        s.dtr = False
        time.sleep(0.1)
        s.dtr = True
        s.close()
        print("✓ Reset triggered!")
    except Exception as e:
        print(f"ERROR: {e}")

env.AddCustomTarget(
    name="reset",
    dependencies=None,
    actions=[reset_esp32],
    title="Reset ESP32",
    description="Reset ESP32 via DTR signal"
)


def ota_deploy(source, target, env):
    """Build manifest.json and copy firmware + manifest to Home Assistant /local"""

    firmware = os.path.join(env.subst("$BUILD_DIR"), env.subst("$PROGNAME") + ".bin")
    if not os.path.isfile(firmware):
        print(f"ERROR: firmware not found: {firmware}")
        env.Exit(1)

    main_cpp = os.path.join(env.subst("$PROJECT_SRC_DIR"), "main.cpp")
    with open(main_cpp, encoding="utf-8") as f:
        match = re.search(r'const String rel = "([^"]+)"', f.read())
    if not match:
        print(f"ERROR: could not find 'const String rel = \"...\"' in {main_cpp}")
        env.Exit(1)
    version = match.group(1)

    with open(firmware, "rb") as f:
        firmware_bytes = f.read()
    manifest = {
        "version": version,
        "file": "firmware.bin",
        "md5": hashlib.md5(firmware_bytes).hexdigest(),
        "size": len(firmware_bytes),
    }
    manifest_path = os.path.join(env.subst("$BUILD_DIR"), "manifest.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print(f"OTA deploy: version {version}, {manifest['size']} bytes, md5 {manifest['md5']}")
    print(f"  -> {OTA_HOST}:{OTA_REMOTE_DIR} (port {OTA_PORT})")

    # firmware.bin first, manifest.json last: the manifest never points to an
    # incomplete binary
    commands = [
        ["ssh", "-p", OTA_PORT, OTA_HOST, f"mkdir -p {OTA_REMOTE_DIR}"],
        ["scp", "-P", OTA_PORT, firmware, f"{OTA_HOST}:{OTA_REMOTE_DIR}/firmware.bin"],
        ["scp", "-P", OTA_PORT, manifest_path, f"{OTA_HOST}:{OTA_REMOTE_DIR}/manifest.json"],
    ]
    for cmd in commands:
        print(f"  $ {' '.join(cmd)}")
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print(f"ERROR: command failed with exit code {result.returncode}")
            print("Check SSH key access to the HA Terminal & SSH addon, or copy")
            print(f"firmware.bin and manifest.json manually (Samba share: config/www/higrow/)")
            env.Exit(1)

    base_url = "https://" + OTA_HOST.split("@")[-1] + ":8123/local/higrow"
    print(f"OK - deployed! Sensors will pick up {version} from {base_url}/manifest.json")

env.AddCustomTarget(
    name="ota_deploy",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=[ota_deploy],
    title="OTA Deploy",
    description="Build firmware, write manifest.json and copy both to Home Assistant"
)
