#!/usr/bin/env python3
"""
buddy_pack.py — Convert character-pack GIF folders to .pack binary for Watchy Buddy,
                and upload packs over BLE.

Usage:
  python buddy_pack.py pack   <input-dir> <output.pack>   [--preview]
  python buddy_pack.py upload <file.pack> [--name <name>]
  python buddy_pack.py list
"""
import argparse
import asyncio
import json
import os
import struct
import sys
import zlib
from pathlib import Path

from PIL import Image

# .pack format constants
PACK_MAGIC   = b"BPACK\x00\x00\x00"
PACK_VERSION = 1
FRAME_W      = 96
FRAME_H      = 96
HEADER_SIZE  = 32
FRAME_ENTRY  = 16   # bytes per frame table entry

# BuddyMood enum values (must match BuddyCharacter.h)
STATE_MAP = {
    "sleep":     4,
    "idle":      1,
    "busy":      2,
    "attention": 3,
    "celebrate": 5,
    "dizzy":     0,   # mapped to OFFLINE slot (reuse for dizzy)
    "heart":     6,
}

NUS_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"


def gif_to_bitmap(gif_path: Path) -> bytes:
    """Convert a GIF (first frame) to 96x96 1-bit bitmap bytes for GxEPD2 drawBitmap.

    PIL mode '1' tobytes(): packed 8 pixels/byte, MSB first. bit=0 → black, bit=1 → white.
    GxEPD2 drawBitmap: bit=1 → draw foreground (BLACK), bit=0 → transparent (WHITE bg).
    So: XOR 0xFF to invert.
    """
    img = Image.open(gif_path)
    frame = img.convert("RGBA")
    bg = Image.new("RGBA", frame.size, (255, 255, 255, 255))
    bg.paste(frame, mask=frame.split()[3])
    bw = bg.convert("L").resize((FRAME_W, FRAME_H), Image.LANCZOS).convert("1")
    raw = bw.tobytes()  # 1152 bytes (96*96/8), already packed MSB-first
    return bytes(b ^ 0xFF for b in raw)


def cmd_pack(args):
    input_dir = Path(args.input_dir)
    manifest_path = input_dir / "manifest.json"
    if not manifest_path.exists():
        print(f"Error: {manifest_path} not found")
        sys.exit(1)

    with open(manifest_path) as f:
        manifest = json.load(f)

    pack_name = manifest.get("name", input_dir.name)
    print(f"Packing '{pack_name}' from {input_dir}")

    frames = []  # list of (state_id, variant, bitmap_bytes)

    for state_name, files in manifest.get("states", {}).items():
        state_id = STATE_MAP.get(state_name)
        if state_id is None:
            print(f"  [skip] unknown state '{state_name}'")
            continue

        if isinstance(files, str):
            files = [files]

        for variant, fname in enumerate(files):
            gif_path = input_dir / fname
            if not gif_path.exists():
                print(f"  [skip] {fname} not found")
                continue

            bitmap = gif_to_bitmap(gif_path)
            frames.append((state_id, variant, bitmap))
            print(f"  [{state_name}] variant {variant}: {fname} -> {len(bitmap)} bytes")

    if not frames:
        print("Error: no frames produced")
        sys.exit(1)

    # Build .pack binary
    num_frames = len(frames)
    data_offset = HEADER_SIZE + FRAME_ENTRY * num_frames

    header = struct.pack("<8sIIHH12s",
        PACK_MAGIC, PACK_VERSION, num_frames, FRAME_W, FRAME_H, b"\x00" * 12)

    frame_table = b""
    data_blob = b""
    for state_id, variant, bitmap in frames:
        offset = len(data_blob)
        size = len(bitmap)
        frame_table += struct.pack("<BBHIIHH",
            state_id, variant, 0, offset, size, 0, 0)  # 16 bytes: +2 padding
        data_blob += bitmap

    pack_bytes = header + frame_table + data_blob
    crc = zlib.crc32(pack_bytes) & 0xFFFFFFFF

    output_path = Path(args.output)
    with open(output_path, "wb") as f:
        f.write(pack_bytes)

    print(f"\nWrote {output_path} ({len(pack_bytes)} bytes, {num_frames} frames, CRC32={crc:08X})")

    if args.preview:
        preview_dir = output_path.parent / f"{pack_name}_preview"
        preview_dir.mkdir(exist_ok=True)
        for state_id, variant, bitmap in frames:
            state_name = [k for k, v in STATE_MAP.items() if v == state_id]
            name = state_name[0] if state_name else str(state_id)
            # Invert back to PIL convention (bit=0 black) then unpack
            pil_bytes = bytes(b ^ 0xFF for b in bitmap)
            img = Image.frombytes("1", (FRAME_W, FRAME_H), pil_bytes)
            img.save(preview_dir / f"{name}_{variant}.png")
        print(f"Previews saved to {preview_dir}/")

    return pack_bytes, crc


async def cmd_upload(args):
    from bleak import BleakScanner, BleakClient

    pack_path = Path(args.file)
    pack_name = args.name or pack_path.stem
    pack_data = pack_path.read_bytes()
    pack_crc = zlib.crc32(pack_data) & 0xFFFFFFFF

    print(f"Pack: {pack_path.name} ({len(pack_data)} bytes, CRC32={pack_crc:08X})")
    print(f"Name: {pack_name}")

    print("Scanning for Claude-Watchy-*...")
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or "").lower().startswith("claude"), timeout=20.0)
    if not device:
        print("Device not found")
        return

    print(f"Found: {device.name} {device.address}")
    responses = []

    def on_notify(sender, data):
        msg = data.decode("utf-8", "replace").strip()
        if msg:
            responses.append(msg)
            print(f"  <- {msg}")

    async with BleakClient(device, timeout=30.0) as client:
        await client.start_notify(NUS_TX, on_notify)
        print("Connected. Starting upload...")

        # pack_begin
        chunk_size = 180  # safe for default MTU
        begin_cmd = json.dumps({
            "cmd": "pack_begin",
            "name": pack_name,
            "size": len(pack_data),
            "crc": pack_crc
        }) + "\n"
        await client.write_gatt_char(NUS_RX, begin_cmd.encode(), response=False)
        await asyncio.sleep(0.3)

        # pack_chunk — send in hex-encoded chunks
        total = len(pack_data)
        seq = 0
        sent = 0
        while sent < total:
            end = min(sent + chunk_size, total)
            hex_data = pack_data[sent:end].hex()
            chunk_cmd = json.dumps({
                "cmd": "pack_chunk",
                "seq": seq,
                "data": hex_data
            }) + "\n"
            await client.write_gatt_char(NUS_RX, chunk_cmd.encode(), response=False)
            sent = end
            seq += 1
            pct = sent * 100 // total
            print(f"\r  Uploading: {pct}% ({sent}/{total} bytes, chunk {seq})", end="", flush=True)
            await asyncio.sleep(0.05)

        print()

        # pack_end — wait for device to finalize file before selecting
        end_cmd = json.dumps({"cmd": "pack_end"}) + "\n"
        await client.write_gatt_char(NUS_RX, end_cmd.encode(), response=False)
        await asyncio.sleep(3)  # give device time to close file + flush FFat

        # pack_select
        sel_cmd = json.dumps({"cmd": "pack_select", "name": pack_name}) + "\n"
        await client.write_gatt_char(NUS_RX, sel_cmd.encode(), response=False)
        await asyncio.sleep(2)

        await client.stop_notify(NUS_TX)
        print("Done.")


async def cmd_list(args):
    from bleak import BleakScanner, BleakClient

    print("Scanning...")
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or "").lower().startswith("claude"), timeout=20.0)
    if not device:
        print("Device not found")
        return

    responses = []
    def on_notify(sender, data):
        msg = data.decode("utf-8", "replace").strip()
        if msg:
            responses.append(msg)
            print(f"  <- {msg}")

    async with BleakClient(device, timeout=30.0) as client:
        await client.start_notify(NUS_TX, on_notify)
        await client.write_gatt_char(NUS_RX, b'{"cmd":"pack_list"}\n', response=False)
        await asyncio.sleep(2)
        await client.stop_notify(NUS_TX)


def main():
    parser = argparse.ArgumentParser(description="Watchy Buddy character pack tool")
    sub = parser.add_subparsers(dest="command")

    p_pack = sub.add_parser("pack", help="Convert GIF folder to .pack")
    p_pack.add_argument("input_dir")
    p_pack.add_argument("output")
    p_pack.add_argument("--preview", action="store_true", help="Save PNG previews")

    p_upload = sub.add_parser("upload", help="Upload .pack to Watchy via BLE")
    p_upload.add_argument("file")
    p_upload.add_argument("--name", default=None)

    p_list = sub.add_parser("list", help="List packs installed on Watchy")

    args = parser.parse_args()

    if args.command == "pack":
        cmd_pack(args)
    elif args.command == "upload":
        asyncio.run(cmd_upload(args))
    elif args.command == "list":
        asyncio.run(cmd_list(args))
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
