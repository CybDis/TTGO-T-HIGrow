<<<<<<< HEAD
import hashlib
import json
import os
import re
import subprocess
import time

=======
import os
>>>>>>> f531aa2 (Linux Anpassungen)
import serial
import serial.tools.list_ports

Import("env")

<<<<<<< HEAD
# OTA deploy destination (Home Assistant with Terminal & SSH addon, key-based auth)
OTA_HOST = "root@192.168.1.6"
OTA_PORT = "22"
OTA_REMOTE_DIR = "/config/www/higrow"

def reset_esp32(source, target, env):
    """PlatformIO custom target to reset ESP32 via DTR"""

    # Find serial port
=======
KNOWN_PORT_PATTERNS = ("ttyACM", "ttyUSB", "cu.SLAB", "usbserial", "COM")


def find_esp32_port(preferred_port=None):
>>>>>>> f531aa2 (Linux Anpassungen)
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None, ports

    # Use preferred port if specified
    if preferred_port:
        for p in ports:
            if p.device == preferred_port or preferred_port in p.device:
                return p.device, ports

    # Look for known patterns (ttyACM, ttyUSB, etc.)
    candidates = []
    for p in ports:
        if any(pattern in p.device for pattern in KNOWN_PORT_PATTERNS):
            candidates.append(p.device)

    if candidates:
        return candidates[0], ports

    return ports[0].device, ports


def reset_esp32(source, target, env):
    """PlatformIO custom target to reset ESP32 via DTR"""

    preferred_port = env.get("ESP32_PORT") or os.environ.get("ESP32_PORT")
    port, ports = find_esp32_port(preferred_port)
    if not port:
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
        print(f"ERROR: Could not configure port: {e}")
        if ports:
            print("\nAvailable ports:")
            for p in ports:
                print(f"  - {p.device}: {p.description}")


env.AddCustomTarget(
    name="reset",
    dependencies=None,
    actions=[reset_esp32],
    title="Reset ESP32",
    description="Reset ESP32 via DTR signal"
)


<<<<<<< HEAD
def ota_deploy(source, target, env):
    """Build manifest.json and copy firmware + manifest to Home Assistant /local"""
=======
def reset_and_monitor_target(source, target, env):
    """PlatformIO custom target: reset ESP32 then open serial monitor by running the helper script.

    Reads `ESP32_PORT` and `ESP32_BAUD` from the environment or falls back to auto-detect and 115200.
    """
    import subprocess
    import sys as _sys

    preferred_port = env.get("ESP32_PORT") or os.environ.get("ESP32_PORT")
    port, ports = find_esp32_port(preferred_port)
    if not port:
        print("ERROR: No serial ports found!")
        return

    # Trigger reset
    print(f"Triggering reset on {port}...")
    try:
        # reuse the reset_esp32 implementation (it accepts source, target, env)
        reset_esp32(None, None, env)
    except Exception as e:
        print(f"Warning: reset failed: {e}")

    # Determine baud
    baud = env.get("ESP32_BAUD") or os.environ.get("ESP32_BAUD") or 115200

    # Run the monitor helper script located in the project root if present
    script_path = os.path.join(os.getcwd(), "reset_and_monitor.py")
    if os.path.exists(script_path):
        print(f"Opening monitor (script) on {port} at {baud} using Python interpreter {_sys.executable}")
        try:
            subprocess.call([_sys.executable, script_path, port, str(baud)])
        except Exception as e:
            print(f"ERROR: Failed to launch monitor script: {e}")
    else:
        print(f"Monitor script not found at {script_path}.\nPlease run the monitor manually or add reset_and_monitor.py to the project.")


env.AddCustomTarget(
    name="reset_monitor",
    dependencies=None,
    actions=[reset_and_monitor_target],
    title="Reset and Monitor ESP32",
    description="Reset ESP32 and open serial monitor (uses reset_and_monitor.py in project root)"
)


>>>>>>> f531aa2 (Linux Anpassungen)

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
