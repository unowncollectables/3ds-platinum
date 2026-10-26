#!/usr/bin/env python3
"""Blocks ROMs, extracted game data, builds, saves, ripped images and secrets from this repo.
  python3 scripts/check_clean.py            check the files in the index (pre-commit) or the tree (CI)
  python3 scripts/check_clean.py --history  also check every blob on every branch and tag
Exit code 1 and a list of problems if anything is found. Allowed exceptions are in ALLOW (keep it tiny)."""
import re, subprocess, sys

BAD_EXT = re.compile(r"\.(nds|srl|gba|sav|dsv|bin|pak|narc|sdat|sseq|sbnk|swar|nclr|ncgr|nscr|ncer|nanr|nsbmd|nsbtx|nsbca|"
                     r"3dsx|cia|3ds|cxi|elf|smdh|bnr|icn|bcwav|bcstm|wav|mp3|ogg|bmp|png|jpe?g|gif|raw|zip|7z|rar|tar|gz)$", re.I)
ALLOW = {"tools/banner3d/art/logo.png", "tools/banner3d/art/portal.png", "platinum3ds/full/cia/chime.wav"}  # GPL art from GabeSud0/pokeplatinum-3ds, see THIRD_PARTY.md
MAX = 1_000_000
MAGIC = [b"NARC", b"SDAT", b"RLCN", b"RGCN", b"RECN", b"RCSN", b"BMD0", b"BTX0", b"3DSX", b"NCSD", b"NCCH"]
SECRET = re.compile(rb"gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}|AKIA[0-9A-Z]{16}|-----BEGIN [A-Z ]*PRIVATE KEY|"
                    rb"xox[baprs]-[A-Za-z0-9-]{10,}|sk-[A-Za-z0-9]{32,}")

def git(*a):
    return subprocess.run(["git", *a], capture_output=True, check=True).stdout

def problems(path, data):
    out = []
    if path in ALLOW:
        return out
    if BAD_EXT.search(path):
        out.append("blocked file type")
    if len(data) > MAX:
        out.append("bigger than 1 MB (%d bytes)" % len(data))
    if data[:4] in MAGIC:
        out.append("looks like DS/3DS game data (%r header)" % data[:4])
    if len(data) > 0x160 and re.match(rb"[A-Z0-9]{4}$", data[12:16]) and data[0x15C:0x15E] == b"\x56\xcf":
        out.append("looks like a DS ROM")
    if SECRET.search(data):
        out.append("contains something that looks like a token or private key")
    return out

def main():
    history = "--history" in sys.argv
    staged = not history and git("diff", "--cached", "--name-only")
    bad = []
    if history:
        seen = set()
        for line in git("rev-list", "--all", "--objects").decode().splitlines():
            sha, _, path = line.partition(" ")
            if not path or sha in seen or git("cat-file", "-t", sha).strip() != b"blob":
                continue
            seen.add(sha)
            for p in problems(path, git("cat-file", "blob", sha)):
                bad.append("%s (blob %s): %s" % (path, sha[:10], p))
    else:
        files = (git("diff", "--cached", "--name-only", "--diff-filter=AM") if staged else git("ls-files")).decode().splitlines()
        for f in files:
            data = git("show", (":" if staged else "HEAD:") + f)   # the committed bytes (a symlink is checked as its link text)
            for p in problems(f, data):
                bad.append("%s: %s" % (f, p))
    if bad:
        print("NOT CLEAN - remove these before committing (never commit ROMs, game data, builds, saves or secrets):")
        print("\n".join("  " + b for b in bad))
        return 1
    print("clean")
    return 0

if __name__ == "__main__":
    sys.exit(main())
