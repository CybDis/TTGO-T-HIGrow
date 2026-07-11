#!/usr/bin/env python3
"""Reset ESP32 via DTR then open a simple serial monitor session.

Usage: python3 reset_and_monitor.py [PORT] [BAUD]
If PORT is omitted the script will try to auto-detect the ESP32 port.
"""
import sys
import threading
import time

import serial
import reset_esp32


def monitor(port, baud=115200):
    try:
        s = serial.Serial(port, baud, timeout=0.1)
    except Exception as e:
        print(f"ERROR: Could not open {port} at {baud}: {e}")
        return

    stop = threading.Event()

    def reader():
        try:
            while not stop.is_set():
                data = s.read(1024)
                if data:
                    try:
                        print(data.decode(errors="replace"), end="", flush=True)
                    except Exception:
                        print(data)
                else:
                    time.sleep(0.01)
        except Exception as e:
            print(f"\nReader error: {e}")
        finally:
            stop.set()

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    try:
        while not stop.is_set():
            line = sys.stdin.readline()
            if line == "":
                break
            try:
                s.write(line.encode())
            except Exception:
                pass
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        try:
            s.close()
        except Exception:
            pass
        time.sleep(0.05)


def main():
    preferred = sys.argv[1] if len(sys.argv) > 1 else None
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    port, _ = reset_esp32.find_esp32_port(preferred)
    if not port:
        print("ERROR: No serial port found.")
        sys.exit(1)

    print(f"Triggering reset on {port}...")
    ok = reset_esp32.reset_esp32(port)
    if not ok:
        print("Warning: Reset failed or could not be triggered; continuing to open monitor.")

    print(f"Opening serial monitor on {port} at {baud}. Press Ctrl-C to exit.")
    monitor(port, baud)


if __name__ == "__main__":
    main()
