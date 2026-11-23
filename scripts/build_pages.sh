#!/usr/bin/env bash
# Compile ESP32 firmware and prepare static website and installer files in web/.
set -Eeuo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

# The original full-length GIF belongs in media/ for GitHub README.
# Remove the obsolete web/ copy left behind by older revisions.
OLD_WEB_GIF="web/assets/pocket-arcade-new-games.gif"
if [[ -f "$OLD_WEB_GIF" ]]; then
  rm -f -- "$OLD_WEB_GIF"
  echo "Removed obsolete website GIF: $OLD_WEB_GIF"
fi

check_web_asset_sizes() {
  python3 - <<'ASSET_CHECK'
from pathlib import Path

limit = 25 * 1024 * 1024
oversized = [(p, p.stat().st_size) for p in Path('web').rglob('*')
             if p.is_file() and p.stat().st_size > limit]
if oversized:
    for path, size in oversized:
        print(f'Website asset too large: {path} ({size / (1024 * 1024):.1f} MiB, limit 25 MiB)')
    raise SystemExit('Remove or replace oversized files in web/ before building.')
ASSET_CHECK
}

# Fail early when a website asset exceeds the static asset size limit.
check_web_asset_sizes

VENV="${TMPDIR:-/tmp}/pocket-arcade-build-venv"
python3 -m venv "$VENV"
"$VENV/bin/python" -m pip install --disable-pip-version-check --quiet 'platformio==6.1.18' 'esptool==4.8.1'
"$VENV/bin/pio" run -e crowpanel_28

BUILD_DIR='.pio/build/crowpanel_28'
APP="$BUILD_DIR/firmware.bin"
BOOT="$BUILD_DIR/bootloader.bin"
PARTITIONS="$BUILD_DIR/partitions.bin"
FRAMEWORK="$HOME/.platformio/packages/framework-arduinoespressif32"
BOOT_APP="$FRAMEWORK/tools/partitions/boot_app0.bin"

for file in "$APP" "$BOOT" "$PARTITIONS" "$BOOT_APP"; do
  if [[ ! -s "$file" ]]; then
    echo "Missing ESP32 build artifact: $file" >&2
    exit 1
  fi
done

TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT
FACTORY="$TEMP_DIR/factory.bin"

"$VENV/bin/python" -m esptool --chip esp32 merge_bin \
  -o "$FACTORY" --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 "$BOOT" \
  0x8000 "$PARTITIONS" \
  0xe000 "$BOOT_APP" \
  0x10000 "$APP"

# Reject invalid firmware images, offsets, sizes or partition layouts.
"$VENV/bin/python" - "$FACTORY" "$APP" <<'PY'
import hashlib
import json
import pathlib
import re
import shutil
import sys

factory = pathlib.Path(sys.argv[1])
application = pathlib.Path(sys.argv[2])
source = pathlib.Path('include/AppConfig.h').read_text(encoding='utf-8')
match = re.search(r'APP_VERSION\s*=\s*"([^"\n]+)"', source)
if not match:
    raise SystemExit('Cannot determine firmware version from AppConfig.h')
version = match.group(1)

factory_bytes = factory.read_bytes()
app_bytes = application.read_bytes()
if not (0x10000 < len(factory_bytes) <= 4 * 1024 * 1024):
    raise SystemExit('Factory image exceeds the 4MB ESP32 flash or is incomplete')
if not (0 < len(app_bytes) <= 0x300000):
    raise SystemExit('Application image exceeds the huge_app partition')
if factory_bytes[0x1000] != 0xE9 or factory_bytes[0x10000] != 0xE9 or app_bytes[0] != 0xE9:
    raise SystemExit('ESP32 bootloader/application magic bytes are invalid')
if factory_bytes[0x8000:0x8002] != b'\xAA\x50':
    raise SystemExit('Missing partition table at offset 0x8000')
if factory_bytes[0x10000:0x10000 + len(app_bytes)] != app_bytes:
    raise SystemExit('Application image does not match the merged factory image')

output = pathlib.Path('web')
firmware_dir = output / 'firmware'
if firmware_dir.exists():
    shutil.rmtree(firmware_dir)
firmware_dir.mkdir(parents=True)

def publish_binary(contents, kind):
    digest = hashlib.sha256(contents).hexdigest()
    name = f'pocket-arcade-{kind}-{digest[:12]}.bin'
    (firmware_dir / name).write_bytes(contents)
    print(f'Published {name}: {len(contents):,} bytes; SHA256 {digest}')
    return f'firmware/{name}'

factory_path = publish_binary(factory_bytes, 'factory')
update_path = publish_binary(app_bytes, 'update')

for name, path, offset in (
    ('manifest-factory.json', factory_path, 0),
    ('manifest-update.json', update_path, 0x10000),
):
    manifest = {
        'name': 'CrowPanel Pocket Arcade',
        'version': version,
        'new_install_prompt_erase': False,
        'builds': [{
            'chipFamily': 'ESP32',
            'improv': False,
            'parts': [{'path': path, 'offset': offset}],
        }],
    }
    (output / name).write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    assert (output / path).is_file()
    print(f'Validated {name}: version {version}, offset {offset:#x}')

print('Website and installer files: web/')
PY

# Verify the final website output after firmware and manifests are published.
rm -f -- "$OLD_WEB_GIF"
check_web_asset_sizes
