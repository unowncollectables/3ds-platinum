#!/usr/bin/env python3
"""Check that a locally owned ROM is the supported Pokémon Platinum (US Rev 1).

extract_rom.py imports SUPPORTED from here. No ROM data is uploaded or changed.
"""
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

SUPPORTED = {
    "0862ec35b24de5c7e2dcb88c9eea0873110d755c": "Pokemon Platinum (US, Rev 1)",
}


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify_rom.py ROM", file=sys.stderr)
        return 2
    sha1 = hashlib.sha1(Path(sys.argv[1]).read_bytes()).hexdigest()
    if sha1 in SUPPORTED:
        print(f"ok: {SUPPORTED[sha1]} ({sha1})")
        return 0
    print(f"unsupported ROM (SHA1 {sha1})", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
