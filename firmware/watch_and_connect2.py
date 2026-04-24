"""Read Watchy serial (no DTR reset) while attempting BLE connect."""
import asyncio
import serial
import threading
from bleak import BleakScanner, BleakClient

NUS_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

stop = False
def serial_reader():
    try:
        # Open without DTR/RTS assertion to avoid resetting S3 via USB-CDC control lines
        ser = serial.Serial()
        ser.port = "COM6"
        ser.baudrate = 115200
        ser.dtr = False
        ser.rts = False
        ser.timeout = 0.2
        ser.open()
        print(f"[SER] opened COM6 (DTR/RTS off)")
        while not stop:
            data = ser.read(512)
            if data:
                text = data.decode("utf-8", "replace")
                for line in text.splitlines():
                    if line.strip():
                        print(f"[SER] {line}")
        ser.close()
    except Exception as e:
        print(f"[SER] error: {e}")

async def ble_run():
    await asyncio.sleep(1)  # let serial start
    print("\n[BLE] scanning (25s)...")
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or "").lower().startswith("claude"), timeout=25.0)
    if not device:
        print("[BLE] NOT FOUND")
        return
    print(f"[BLE] found {device.name} {device.address}")
    print("[BLE] connecting...")
    try:
        async with BleakClient(device, timeout=45.0) as c:
            print(f"[BLE] CONNECTED. MTU={c.mtu_size}")
            for svc in c.services:
                print(f"[BLE]   svc {svc.uuid}")
                for ch in svc.characteristics:
                    print(f"[BLE]     char {ch.uuid} props={ch.properties}")
            await c.write_gatt_char(NUS_RX, b'{"cmd":"status"}\n', response=False)
            print("[BLE] wrote status cmd")
            await asyncio.sleep(3)
    except Exception as e:
        print(f"[BLE] ERROR: {type(e).__name__}: {e}")

async def main():
    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()
    await ble_run()
    await asyncio.sleep(2)

asyncio.run(main())
