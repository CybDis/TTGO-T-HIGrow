#!/usr/bin/env python3
import os
import sys
import serial
import serial.tools.list_ports
import time

KNOWN_PORT_PATTERNS = ("ttyACM", "ttyUSB", "cu.SLAB", "usbserial", "COM")


def find_esp32_port(preferred_port=None):
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None, ports

    # Debug: Print all available ports
    print("DEBUG: Available serial ports:")
    for p in ports:
        print(f"  - {p.device}: {p.description}")

    # Use preferred port if specified
    if preferred_port:
        for p in ports:
            if p.device == preferred_port or preferred_port in p.device:
                print(f"DEBUG: Using preferred port {p.device}")
                return p.device, ports

    # Look for known patterns (ttyACM, ttyUSB, etc.)
    candidates = []
    for p in ports:
        if any(pattern in p.device for pattern in KNOWN_PORT_PATTERNS):
            candidates.append(p.device)

    if candidates:
        print(f"DEBUG: Using known device pattern: {candidates[0]}")
        return candidates[0], ports

    # Fallback to first available port
    print(f"DEBUG: Using fallback first port: {ports[0].device}")
    return ports[0].device, ports


def reset_esp32(port_hint=None):
    """Trigger ESP32 reset via DTR signal"""
    preferred_port = port_hint or os.environ.get("ESP32_PORT")
    port, ports = find_esp32_port(preferred_port)

    if not port:
        print("ERROR: No serial ports found!")
        return False

    print(f"Resetting ESP32 on {port}...")

    try:
        s = serial.Serial(port, 115200, timeout=1)
        s.dtr = False
        time.sleep(0.1)
        s.dtr = True
        s.close()
        print("✓ Reset triggered!")
        return True
    except Exception as e:
        print(f"ERROR: Could not configure port: {e}")
        if ports:
            print("\nAvailable ports:")
            for p in ports:
                print(f"  - {p.device}: {p.description}")
        return False


if __name__ == "__main__":
    arg = sys.argv[1] if len(sys.argv) > 1 else None
    reset_esp32(arg)
