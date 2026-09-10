#!/usr/bin/env python3
"""Package the existing local build with explicit playtest settings."""
from pathlib import Path
import plistlib
import shutil

root = Path(__file__).resolve().parent.parent
app = root / 'build/RA Widescreen Playtest.app'
macos = app / 'Contents/MacOS'
macos.mkdir(parents=True, exist_ok=True)
resources = app / 'Contents/Resources'
resources.mkdir(parents=True, exist_ok=True)
shutil.copy2(root / 'packaging/icons/redalert.icns', resources / 'redalert.icns')
# Atomic replacement leaves an already-running executable untouched.
pending = macos / 'redalert_mac.next'
shutil.copy2(root / 'build/redalert_mac', pending)
pending.replace(macos / 'redalert_mac')
assets = macos / 'assets'
if not assets.exists():
    assets.symlink_to(root / 'assets', target_is_directory=True)
shutil.copytree(root / 'presentation', macos / 'presentation', dirs_exist_ok=True)
launcher = macos / 'launch-playtest'
launcher.write_text('''#!/bin/sh
cd -- "$(dirname -- "$0")" || exit 1
export RA_FULLSCREEN="${RA_FULLSCREEN:-1}"
export RA_WIDESCREEN="${RA_WIDESCREEN:-1}"
export RA_CRT="${RA_CRT:-1}"
export RA_TITLE_ART="${RA_TITLE_ART:-wide}"
printf 'Launch: fullscreen=%s widescreen=%s crt=%s title=%s\\n' "$RA_FULLSCREEN" "$RA_WIDESCREEN" "$RA_CRT" "$RA_TITLE_ART" > playtest.log
exec ./redalert_mac "$@" >> playtest.log 2>&1
''')
launcher.chmod(0o755)
info = {
    'CFBundleExecutable': 'launch-playtest',
    'CFBundleIdentifier': 'local.raport.widescreen-playtest.launcher',
    'CFBundleName': 'RA Widescreen Playtest',
    'CFBundleIconFile': 'redalert.icns',
    'CFBundlePackageType': 'APPL',
    'CFBundleVersion': '2',
}
(app / 'Contents/Info.plist').write_bytes(plistlib.dumps(info))
print(app)
