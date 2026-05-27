#!/usr/bin/env python3
import serial
import serial.tools.list_ports
import time

def reset_esp32():
    """Trigger ESP32 reset via DTR signal"""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found!")
        return False
    
    port = ports[0].device
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
        print(f"Error: {e}")
        return False

if __name__ == "__main__":
    reset_esp32()
