#!/usr/bin/env python3
"""platinum3ds (from SoulSilver ss3ds 0181): make the staged compile-network.py safe to re-run on an already built tree.

compile-network.py edits the staged libntrdwc sources in place (0172/0167 dwc_backup.c, 0177 dwc_connectinet.c,
0178/0179 dwc_ac.c) and exits when an anchor is missing. Those files keep earlier edits, so a re-run (0180) stopped
with "ss3ds 0167: expected dwc_backup.c text not found". This rewrites only the STAGED copy so each edit is skipped
when its result is already there; on a pristine tree it behaves exactly as before. Fails loudly if an anchor moved."""
import sys
path = sys.argv[1]
s = open(path).read()
edits = [
    # 0172: _dwc_replace(old, new) -> skip when the new text is already in the file
    ("    if old not in _dwc_src:\n",
     "    if new in _dwc_src:\n        return\n    if old not in _dwc_src:\n"),
    # 0177: bounded CleanupInet loop
    ("if _ci_src.count(_ci_old) != 1:\n",
     "if _ci_new in _ci_src:\n    _ci_old = _ci_new\nelif _ci_src.count(_ci_old) != 1:\n"),
    # 0178: AC bypass
    ("if _ac_src.count(_ac_old1) != 1 or _ac_src.count(_ac_old2) != 1:\n",
     "if 'ss3ds 0178' in _ac_src:\n    _ac_old1 = _ac_new1 = _ac_old2 = _ac_new2 = '\\x00'\nelif _ac_src.count(_ac_old1) != 1 or _ac_src.count(_ac_old2) != 1:\n"),
    # 0179: one-time marker
    ("if _ac_src.count(_ac2_old) != 1:\n",
     "if 'ss3ds 0179' in _ac_src:\n    _ac2_old = _ac2_new = '\\x00'\nelif _ac_src.count(_ac2_old) != 1:\n"),
]
for old, new in edits:
    if s.count(old) != 1:
        sys.exit("ss3ds 0181: compile-network.py anchor not found exactly once: %r" % old.strip())
    s = s.replace(old, new, 1)
open(path, "w").write(s)
