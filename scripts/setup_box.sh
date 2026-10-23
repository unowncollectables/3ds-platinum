#!/bin/bash
# run 73: reinstall build/test tools after a box reset (devkitPro from the official Docker image, Azahar AppImage)
set -e
which make convert >/dev/null 2>&1 || { sudo apt-get update -qq; sudo apt-get install -y -qq make imagemagick >/dev/null; }
if [ ! -d /opt/devkitpro/devkitARM ]; then cd /tmp; R=devkitpro/devkitarm
 T=$(curl -s "https://auth.docker.io/token?service=registry.docker.io&scope=repository:$R:pull" | python3 -c 'import sys,json;print(json.load(sys.stdin)["token"])')
 D=$(curl -s -H "Authorization: Bearer $T" -H "Accept: application/vnd.oci.image.index.v1+json" https://registry-1.docker.io/v2/$R/manifests/latest | python3 -c 'import sys,json;print([m["digest"] for m in json.load(sys.stdin)["manifests"] if m.get("platform",{}).get("architecture")=="amd64"][0])')
 curl -s -H "Authorization: Bearer $T" -H "Accept: application/vnd.oci.image.manifest.v1+json" https://registry-1.docker.io/v2/$R/manifests/$D > m.json
 for d in $(python3 -c 'import json;[print(l["digest"]) for l in json.load(open("m.json"))["layers"]]'); do curl -sL -H "Authorization: Bearer $T" https://registry-1.docker.io/v2/$R/blobs/$d -o l.tgz; sudo tar -xzf l.tgz -C / --wildcards 'opt/devkitpro/*' 2>/dev/null || true; done; rm -f l.tgz; fi
if [ ! -x /opt/azahar/squashfs-root/AppRun ]; then cd /tmp; u=$(curl -sSL https://api.github.com/repos/azahar-emu/azahar/releases/latest | grep -o 'https://[^"]*\.AppImage' | grep -v -i "room\|arm\|aarch" | head -1)
 curl -sSLf -o az.AppImage "$u"; chmod +x az.AppImage; sudo mkdir -p /opt/azahar; sudo chown box:box /opt/azahar; cd /opt/azahar; /tmp/az.AppImage --appimage-extract >/dev/null; fi
echo setup ok
