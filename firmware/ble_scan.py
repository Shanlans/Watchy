import asyncio
from bleak import BleakScanner

NUS_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"

async def main():
    print("Scanning BLE for 15 seconds...")
    devices = await BleakScanner.discover(timeout=15.0, return_adv=True)
    claude_found = []
    nus_found = []
    other_named = []
    for addr, (dev, adv) in devices.items():
        name = dev.name or ""
        uuids = [u.lower() for u in (adv.service_uuids or [])]
        if name.lower().startswith("claude"):
            claude_found.append((name, addr, adv.rssi))
        if NUS_UUID.lower() in uuids:
            nus_found.append((name, addr, adv.rssi))
        if name and name not in [n for n, _, _ in claude_found]:
            other_named.append((name, addr, adv.rssi))

    print()
    print("=== Claude-* devices ===")
    if not claude_found:
        print("  (none)")
    for name, addr, rssi in claude_found:
        print(f"  {name}  {addr}  {rssi}dBm")

    print()
    print("=== Devices advertising Nordic UART Service ===")
    if not nus_found:
        print("  (none)")
    for name, addr, rssi in nus_found:
        print(f"  name='{name}' addr={addr} rssi={rssi}dBm")

    print()
    print(f"=== Other named BLE devices in range ({len(other_named)}) ===")
    for name, addr, rssi in sorted(other_named, key=lambda x: -x[2])[:15]:
        print(f"  {name}  {rssi}dBm")

    print()
    if claude_found or nus_found:
        print("RESULT: Watchy Buddy is advertising correctly.")
    else:
        print("RESULT: No Claude/NUS device found.")

asyncio.run(main())
