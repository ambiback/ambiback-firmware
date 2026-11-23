# Web Build Workflow - Agent Guide

## Overview

The AmbiBack Controller web interface is embedded directly into the firmware as C++ header files. **Source of truth** for the embedded UI is `web/HTML/` (full UI) and `web/HTML_MIN/` (minimal UI). The generated headers `ambiwebserver_html_raw.h` and `ambiwebserver_html_raw_min.h` are **build artifacts** (not committed); they are produced by `convert.py` / `convert.sh` and must exist in the project root before compiling firmware. If they are missing, the build fails on `#include "ambiwebserver_html_raw.h"` (or `_min`).

**Before compiling firmware**: From the `web/` directory run `./convert.sh` (or `bash convert.sh` on Windows). This generates `../ambiwebserver_html_raw.h` and `../ambiwebserver_html_raw_min.h`.

## Workflow Summary

1. **Start Minification Services** (optional, for minified output): WSL minification servers, or rely on Node.js minifiers used by `convert.py`
2. **Convert Files**: From `web/`, run `convert.sh` (or `python3 convert.py` twice). Produces the two header files in project root.
3. **Compile Firmware**: The chosen header is included in `ambiwebserver.cpp` and served via ESP32 web server

## Prerequisites

### Required Software
- **WSL (Windows Subsystem for Linux)**: All build scripts run under WSL
- **Python**: Required by `convert.py` (Python 3 recommended)
- **Redis Server**: Required for minification services
- **Minification Services**: Three local services must be running:
  - JavaScript minifier (port 8021)
  - CSS minifier (port 8031)
  - HTML minifier (port 8041)

### Service Locations
The minification services are located at:
- `/mnt/d/!cvsroot/_Misc/cssminifier.com/runscreen.sh`
- `/mnt/d/!cvsroot/_Misc/html-minifier.com/runscreen.sh`
- `/mnt/d/!cvsroot/_Misc/javascript-minifier.com/runscreen.sh`

## Step-by-Step Build Process

### Step 1: Start Minification Services

**Script**: `web/runminiservers.sh`

**What it does**:
- Installs/ensures Redis server is running
- Sets up screen directory permissions
- Starts three minification services in screen sessions:
  - CSS minifier on port 8031
  - HTML minifier on port 8041
  - JavaScript minifier on port 8021

**Usage** (from WSL):
```bash
cd /mnt/d/!cvsroot/_Misc/AmbiBack.Controller/web
bash runminiservers.sh
```

**Important**: These services must be running before Step 2. The services run in screen sessions, so they persist after the script completes.

### Step 2: Convert HTML Files to Header

**Script**: `web/convert.sh`

**What it does**:
- Calls `convert.py` twice (full + minimal)
- Outputs:
  - `../ambiwebserver_html_raw.h` (full UI)
  - `../ambiwebserver_html_raw_min.h` (minimal UI)

**Usage** (from WSL):
```bash
cd /mnt/d/!cvsroot/_Misc/AmbiBack.Controller/web
bash convert.sh
```

### Step 3: Conversion Process (convert.py)

**Script**: `web/convert.py`

**What it does**:

1. **Scans** the selected input folder (`HTML/` or `HTML_MIN/`) recursively
   - Usage: `python3 convert.py [--no-minify] <output_header> <input_folder>`
   - `--no-minify`: Skip minification (use source as-is). Requires only Python; useful when Node.js or minification services are unavailable (e.g. CI). Produces larger firmware.
2. **Processes each file** based on extension:
   - **`.js` files**: Minified via `http://localhost:8021/raw` → Converted to hex
   - **`.css` files**: Minified via `http://localhost:8031/raw` → Converted to hex
   - **`.html` files**: Minified via `http://localhost:8041/raw` → Converted to hex
   - **`.png`, `.jpg`, `.gif` files**: Converted directly to hex (no minification)
   - **Other files**: Ignored

3. **Generates C++ header** with:
   - Path constants: `static const char* data_<filename>_<ext>_path PROGMEM = "..."` (static avoids multiple-definition when header is included from multiple .cpp files)
   - Hex data arrays: `const char data_<filename>_<ext>[] PROGMEM = {0x..., ...}`
   - Length defines: `#define DATA_<FILENAME>_<EXT>_LEN <count>`

**File Naming Convention**:
- Files in root: `data_<filename>_<ext>`
- Files in subdirectories: `data_<dirname>_<filename>_<ext>`
- Example: `HTML/images/app_logo.png` → `data_images_app_logo_png`

**Error Handling**:
- Retries up to 10 times if minification service fails
- Falls back to uncompressed data if all retries fail
- Files ending in `.min.js` are not minified (assumed already minified)

## Generated Header Structure

The `ambiwebserver_html_raw.h` file contains:

```cpp
// /index.html
static const char* data_index_html_path PROGMEM = "/index.html";
const char data_index_html[] PROGMEM = {0x3c, 0x68, 0x74, ...};
#define DATA_INDEX_HTML_LEN 1234

// /include_ambiback_script.js
static const char* data_include_ambiback_script_js_path PROGMEM = "/include_ambiback_script.js";
const char data_include_ambiback_script_js[] PROGMEM = {0x76, 0x61, 0x72, ...};
#define DATA_INCLUDE_AMBIBACK_SCRIPT_JS_LEN 5678

// ... and so on for all files (path symbols are static to allow header in multiple TUs)
```

## Integration with Web Server

### Header Inclusion Chain

1. `ambiwebserver.cpp` includes:
   ```cpp
   #include "ambiwebserver_html.h"
   ```

2. `ambiwebserver_html.h` includes:
   ```cpp
   #include "ambiwebserver_html_raw.h"
   ```

### Serving Files

The web server (`ambiwebserver.cpp`) registers routes and serves files using:

```cpp
// Register route
pWebServer->on(data_index_html_path, &handleIndex);

// Handler function
void handleIndex(AsyncWebServerRequest *request) {
  request->send_P(200, TEXT_HTML, 
    (const uint8_t *)data_index_html, 
    DATA_INDEX_HTML_LEN, 
    indexProcessor);  // Optional template processor
}
```

**Key Points**:
- `send_P()` reads from PROGMEM (flash memory), not RAM
- Template processors can replace variables (e.g., `{{VERSION}}`)
- MIME types: `TEXT_HTML`, `TEXT_JAVASCRIPT`, `TEXT_CSS`, `IMAGE_PNG`, etc.

### File Types Served

- **HTML**: Main pages and includes (`index.html`, `include_*.html`)
- **JavaScript**: Application scripts and libraries
- **CSS**: Stylesheets
- **Images**: PNG, JPG, GIF files (logos, icons, etc.)

## File Structure

```
web/
├── AGENTS.md              # This file
├── runminiservers.sh      # Start minification services
├── convert.sh             # Wrapper script to run conversion
├── convert.py             # Main conversion script
├── HTML/                  # Source files (edit these!)
├── HTML_MIN/              # Minimal UI source (edit these!)
│   ├── index.html
│   ├── include_*.html
│   ├── include_*.js
│   ├── include_*.css
│   ├── jquery_*.js
│   ├── jquery_*.css
│   └── images/
│       └── *.png, *.gif
└── ESP-HTML-Compressor/   # Alternative compressor (not used)

../                          # Project root
├── ambiwebserver_html_raw.h      # Generated (build artifact, in .gitignore)
├── ambiwebserver_html_raw_min.h  # Generated (build artifact, in .gitignore)
└── ambiwebserver_html.h      # Wrapper header (tracked)

## Minimal UI Build Notes

- Minimal UI is enabled by `EMBED_MINIMAL_UI` in `globalincludes.h`.
- The minimal UI is intentionally tiny and pulls Pico.css from cdnjs at runtime.
- The full UI should live externally (e.g., `https://ambiback.github.io/ambiback-ui/`) and talk to the device API over CORS.
```

## Common Tasks

### Adding a New HTML/CSS/JS File

1. **Add file** to `web/HTML/` (or subdirectory)
2. **Run build process**:
   ```bash
   cd web
   bash runminiservers.sh  # If services not running
   bash convert.sh
   ```
3. **Register route** in `ambiwebserver.cpp`:
   ```cpp
   pWebServer->on(data_<filename>_<ext>_path, HTTP_GET, [] (AsyncWebServerRequest *request){
     request->send_P(200, TEXT_<TYPE>, 
       (const uint8_t *)data_<filename>_<ext>, 
       DATA_<FILENAME>_<EXT>_LEN);
   });
   ```
4. **Recompile** firmware

### Modifying Existing Files

1. **Edit** files in `web/HTML/`
2. **Run conversion**:
   ```bash
   cd web
   bash convert.sh
   ```
3. **Recompile** firmware (no code changes needed if routes already exist)

### Troubleshooting

#### Minification Services Not Responding

**Symptoms**: `convert.py` fails with connection errors

**Solutions**:
1. Check services are running:
   ```bash
   screen -ls  # List screen sessions
   ```
2. Restart services:
   ```bash
   bash runminiservers.sh
   ```
3. Verify ports are accessible:
   ```bash
   curl http://localhost:8021/raw  # Should return error (needs POST)
   ```

#### Python Not Found

**Symptoms**: `convert.sh` fails with "python: command not found"

**Solutions**:
1. Install Python in WSL:
   ```bash
   sudo apt update
   sudo apt install python3 python3-pip
   ```
2. Ensure `python` command is available (may need to create symlink):
   ```bash
   sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 1
   ```
3. Install required Python package:
   ```bash
   pip install requests
   ```

#### Redis Server Not Running

**Symptoms**: `runminiservers.sh` fails

**Solutions**:
```bash
sudo apt install redis-server
sudo service redis-server start
```

#### Generated Header Not Updating

**Symptoms**: Changes to HTML files don't appear in firmware

**Solutions**:
1. Verify conversion ran successfully (check timestamps)
2. Ensure `convert.sh` output path is correct (`../ambiwebserver_html_raw.h`)
3. Check for file permission issues
4. Verify no syntax errors in source files

#### Memory Issues

**Symptoms**: Firmware too large after adding files

**Solutions**:
1. Ensure minification is working (check file sizes in header)
2. Remove unused files from `HTML/` folder
3. Optimize images (compress PNG/JPG before adding)
4. Consider removing unused jQuery plugins

## Important Notes

### Why This Approach?

- **Embedded System**: ESP32 has limited storage, no filesystem
- **Performance**: Files served from flash (PROGMEM) are fast
- **Reliability**: No SD card or external storage required
- **Size Optimization**: Minification reduces firmware size

### Why WSL?

- Minification services are Linux-based
- Scripts use bash and Linux paths
- Services run in screen sessions (Linux feature)

### Why Python?

- `convert.py` uses standard Python libraries (`requests`, `os`, `sys`)
- Script is compatible with Python 3
- Python 3 is recommended for better support and security

### File Size Considerations

- **Maximum Firmware Size**: Varies by ESP32 model (typically 1-4MB)
- **Current Web Assets**: Check `ambiwebserver_html_raw.h` file size
- **Optimization Tips**:
  - Use minified versions of jQuery/plugins
  - Compress images before adding
  - Remove unused CSS/JS
  - Consider code splitting for large features

## Future Improvements

Potential enhancements to the build system:

1. **Automated Build**: Add to CI/CD pipeline
3. **Watch Mode**: Auto-rebuild on file changes
4. **Size Reporting**: Track asset sizes and warn on growth
5. **Alternative Minifiers**: Support for other minification tools
6. **Template System**: Better variable substitution in HTML

## Related Files

- **`ambiwebserver.cpp`**: Web server implementation, route registration
- **`ambiwebserver.h`**: Web server class definition
- **`ambiwebserver_html.h`**: Header wrapper
- **`ambiwebserver_html_raw.h`**: Generated header (auto-generated, do not edit)
- **`web/HTML/`**: Source files (edit these)

---

**Last Updated**: Based on current build system
**Maintainer**: See git history for contributors
