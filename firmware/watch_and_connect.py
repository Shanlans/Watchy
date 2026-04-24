"""Read Watchy serial logs while attempting BLE connect in parallel."""
import asyncio
import serial
import threading
import time
from bleak import BleakScanner, BleakClient

SERIAL_PORT = "COM6"
SERIAL_BAUD = 115200
NUS_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

stop_serial = False

def serial_reader():
    try:
        with serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.2) as ser:
            print(f"[SERIAL {SERIAL_PORT}] opened")
            buf = b""
            while not stop_serial:
                data = ser.read(512)
                if data:
                    buf += data
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        print(f"[SERIAL] {line.decode('utf-8', 'replace').rstrip()}")
    except Exception as e:
        print(f"[SERIAL] error: {e}")

async def ble_attempt():
    await asyncio.sleep(2)  # let serial start first
    print("\n[BLE] scanning...")
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or "").lower().startswith("claude"), timeout=20.0)
    if not device:
        print("[BLE] device not found")
        return
    print(f"[BLE] found: {device.name} {device.address}")
    print("[BLE] connecting (45s)...")
    try:
        async with BleakClient(device, timeout=45.0) as c:
            print(f"[BLE] connected={c.is_connected} mtu={c.mtu_size}")
            for svc in c.services:
                print(f"[BLE]   svc {svc.uuid}")
            await c.write_gatt_char(NUS_RX, b'{"cmd":"status"}\n', response=False)
            print("[BLE] wrote status")
            await asyncio.sleep(3)
    except Exception as e:
        print(f"[BLE] error: {type(e).__name__}: {e}")

async def main():
    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()
    await ble_attempt()
    await asyncio.sleep(2)

asyncio.run(main())
stop_serial = True
time.sleep(0.3)
