#!/usr/bin/env bash
# One time per clone: block unsafe files at commit time.
cd "$(git rev-parse --show-toplevel)" && printf '#!/bin/sh\nexec python3 scripts/check_clean.py\n' > .git/hooks/pre-commit && chmod +x .git/hooks/pre-commit && echo "pre-commit guard installed"
