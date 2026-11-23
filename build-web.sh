#!/bin/bash
# Generate embedded web UI headers from web/HTML and web/HTML_MIN.
# Run from repo root. After this, compile in Arduino IDE (or arduino-cli).

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/web"
bash convert.sh
echo ""
echo "Web headers ready: ambiwebserver_html_raw.h, ambiwebserver_html_raw_min.h"
echo "You can now compile the firmware in Arduino IDE."
