#!/bin/bash
npm install

# Run the conversion script
python3 convert.py ../ambiwebserver_html_raw.h HTML
python3 convert.py ../ambiwebserver_html_raw_min.h HTML_MIN
