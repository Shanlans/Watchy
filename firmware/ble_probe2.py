"""Probe BLE by MAC address with verbose logging."""
import asyncio
import logging
from bleak import BleakClient

logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(name)s %(levelname)s: %(message)s")
logging.getLogger("bleak").setLevel(logging.INFO)  # reduce noise

MAC = "AC:A7:04:2A:1C:29"
NUS_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

async def main():
    print(f"Connecting directly to {MAC} (60s timeout)...")
    try:
        async with BleakClient(MAC, timeout=60.0) as client:
            print(f"  connected: {client.is_connected}")
            print(f"  MTU: {client.mtu_size}")

            print("\nServices:")
            for svc in client.services:
                print(f"  [svc] {svc.uuid}")
                for ch in svc.characteristics:
                    print(f"         [char] {ch.uuid}  props={ch.properties}")

            # Subscribe to notifications
            def on_notify(sender, data):
                msg = data.decode('utf-8', errors='replace').strip()
                print(f"<-- NOTIFY: {msg}")

            await client.start_notify(NUS_TX, on_notify)
            print("\nSubscribed. Sending test data...")

            await client.write_gatt_char(NUS_RX, b'{"cmd":"status"}\n', response=False)
            await asyncio.sleep(2)

            await client.write_gatt_char(NUS_RX,
                b'{"total":2,"running":1,"waiting":0,"msg":"hello from probe","tokens":100,"tokens_today":50}\n',
                response=False)

            print("Waiting 5s for responses...")
            await asyncio.sleep(5)

            await client.stop_notify(NUS_TX)
    except asyncio.TimeoutError:
        print("TIMEOUT")
    except Exception as e:
        print(f"ERROR: {type(e).__name__}: {e}")

asyncio.run(main())
