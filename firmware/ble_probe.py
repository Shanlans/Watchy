"""Probe BLE connection to Watchy Buddy.
Connects, discovers services, writes a fake heartbeat, and listens for notifications.
"""
import asyncio
from bleak import BleakScanner, BleakClient

NUS_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # desktop -> device
NUS_TX      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # device -> desktop

async def main():
    print("Finding Claude-Watchy-*...")
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or "").lower().startswith("claude"),
        timeout=30.0,
    )
    if not device:
        print("NOT FOUND in 10s.")
        return
    print(f"Found: {device.name}  {device.address}")

    print("\nConnecting (30s timeout)...")
    try:
        async with BleakClient(device, timeout=30.0) as client:
            print(f"  connected={client.is_connected}")

            print("\nServices:")
            for svc in client.services:
                print(f"  [svc] {svc.uuid}  {svc.description}")
                for ch in svc.characteristics:
                    props = ",".join(ch.properties)
                    print(f"         [char] {ch.uuid}  props={props}")

            # Subscribe to TX notifications
            def on_notify(sender, data):
                print(f"\n<-- NOTIFY: {data.decode('utf-8', errors='replace').strip()}")

            await client.start_notify(NUS_TX, on_notify)
            print("\nSubscribed to TX.")

            # Send a fake heartbeat
            fake_heartbeat = (
                '{"total":2,"running":1,"waiting":0,'
                '"msg":"test from python","entries":['
                '"19:20 probe_hello","19:19 probe_init","19:18 probe_start"],'
                '"tokens":12345,"tokens_today":6789}'
                "\n"
            )
            print("\n--> Writing fake heartbeat:")
            print(f"    {fake_heartbeat.strip()}")
            await client.write_gatt_char(NUS_RX, fake_heartbeat.encode("utf-8"), response=False)

            # Send a status command
            print("\n--> Writing {cmd:status}")
            await client.write_gatt_char(NUS_RX, b'{"cmd":"status"}\n', response=False)

            print("\nListening for 5 seconds...")
            await asyncio.sleep(5)

            await client.stop_notify(NUS_TX)
            print("\nDone.")
    except Exception as e:
        print(f"CONNECT ERROR: {type(e).__name__}: {e}")

asyncio.run(main())
