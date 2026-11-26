#!/usr/bin/env python3
"""Bake a Discord webhook URL into a built VATPHIL.dll.

The DLL reserves a 256-byte block initialized with a placeholder. This script
replaces that block in the PE file, so the URL is compiled into the DLL instead
of being read from the config. The URL is never stored in the repository.

Usage:
    WEBHOOK_URL='https://discord.com/api/webhooks/...' \
        python3 tools/bake_webhook.py --dll dist/VATPHIL.dll \
            --out dist/VATPHIL_baked.dll

Use the environment variable form so the URL does not end up in shell history.
"""

import argparse
import os
import sys

MARKER = b"https://bake.invalid/00000000-0000-0000-0000-000000000000"
BLOCK = 256


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", required=True, help="unpatched DLL from the build")
    parser.add_argument("--out", required=True, help="patched DLL to write")
    parser.add_argument(
        "--url",
        default=os.environ.get("WEBHOOK_URL"),
        help="webhook URL (or set WEBHOOK_URL)",
    )
    args = parser.parse_args()

    if not args.url:
        print("error: set WEBHOOK_URL or pass --url", file=sys.stderr)
        return 2
    try:
        url = args.url.encode("ascii")
    except UnicodeEncodeError:
        print("error: webhook URL must be ASCII", file=sys.stderr)
        return 2
    if not args.url.startswith("https://"):
        print("error: webhook URL must start with https://", file=sys.stderr)
        return 2
    if b"\0" in url or len(url) >= BLOCK:
        print(f"error: webhook URL must be shorter than {BLOCK} bytes", file=sys.stderr)
        return 2

    with open(args.dll, "rb") as f:
        data = bytearray(f.read())

    offsets = []
    start = 0
    while True:
        offset = data.find(MARKER, start)
        if offset < 0:
            break
        block = data[offset:offset + BLOCK]
        if len(block) == BLOCK and not any(block[len(MARKER):]):
            offsets.append(offset)
        start = offset + 1

    if not offsets:
        print(
            "error: no zero-padded 256-byte webhook placeholder found; "
            "build the DLL from the current source first.",
            file=sys.stderr,
        )
        return 1

    for offset in offsets:
        data[offset:offset + BLOCK] = url + b"\0" * (BLOCK - len(url))

    with open(args.out, "wb") as f:
        f.write(data)

    with open(args.out, "rb") as f:
        patched = f.read()
    for offset in offsets:
        if patched[offset:offset + len(url)] != url:
            print("error: verification failed", file=sys.stderr)
            return 1

    print(f"baked webhook URL into {args.out} ({len(url)} bytes, {len(offsets)} block(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
