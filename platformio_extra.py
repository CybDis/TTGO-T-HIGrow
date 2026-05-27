import serial
import serial.tools.list_ports
import time

Import("env")

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







