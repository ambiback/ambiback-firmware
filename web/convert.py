#!/usr/bin/python
#	 pip install requests
import requests
import sys
import os
import time
import re
import subprocess
import json

input_dir = "HTML"              # Sub folder of webfiles
output_dir = "HTML_minified"

# Parse args: optional --no-minify, then output path, then input folder
args = [a for a in sys.argv[1:] if a != '--no-minify']
no_minify = '--no-minify' in sys.argv

try:
	f_output_path = args[0]
except IndexError:
	f_output_path = 'output_dir'

try:
	input_dir = args[1]
except IndexError:
	pass
input_dir = input_dir.replace(os.sep, '/')  # normalize for consistent paths on Windows/Linux
print("Using outputfile: " + f_output_path)
print("Using input folder: " + input_dir)
if no_minify:
	print("Using --no-minify: skipping minification (Python only)")

f_output = open(f_output_path, "w")
# Legacy HTTP minifier services (fallback only - not used if Node.js available)
URL_minify_js   = 'http://localhost:8021/raw'			# Fallback: local version of javascript minifier
URL_minify_css  = 'http://localhost:8031/raw'			# Fallback: local version of css minifier
URL_minify_html = 'http://localhost:8041/raw'			# Fallback: local version of html minifier

def write_to_file(file, data, dir=""):
    filename, file_extension = os.path.splitext(file)       # Split filename and file extension
    file_extension = file_extension.replace(".","")         # Remove puncuation in file extension

    dir = dir.replace(input_dir,"")                         # Remove the first directory(input_dir)
    dir = dir.replace("\\","/")                             # Chang to /
    f_output.write("// " + dir + "\n")                      # Print comment
    # static so header can be included from multiple TUs without linker multiple-definition errors
    f_output.write("static const char* data_" + filename + "_" + file_extension + "_path PROGMEM = \""+str(dir)+"\";\n")	# print path
    f_output.write("const char data_" + filename + "_" + file_extension + "[] PROGMEM = {"+data.upper()+"};\n")			# print binary data
    f_output.write("#define " + ("data_" + filename + "_" + file_extension + "_len " + str(data.count('0x'))).upper() + "\n\n")

def file2Hex(filename):
    output_str = ""
    x = 1
    myfile = open(filename, "rb")
    try:
        binLen = os.path.getsize(filename)
        byte = myfile.read(1)

        while byte != b"":
            output_str += hex(ord(byte))
            if (x != binLen):
                output_str += ","
            x += 1
            # Next Byte 
            byte = myfile.read(1)
    finally:
        myfile.close()
    return output_str

def aschii2Hex(text):
    output_str = ""
    x = 1
    strLen = len(text)
    for character in text:
        output_str += hex(ord(character))

        if (x != strLen):
            output_str += ","
        x += 1
    return output_str

def minify_js(input_file):
	"""Minify JavaScript using terser via Node.js"""
	# Skip minification for already minified files
	if input_file.endswith("min.js"):
		input_data = open(input_file, 'rb').read()
		if isinstance(input_data, bytes):
			return input_data.decode('utf-8', errors='ignore')
		return input_data
	
	for i in range(0,10):
		try:
			# Read input file
			input_data = open(input_file, 'rb').read()
			# Decode bytes to string
			if isinstance(input_data, bytes):
				js_content = input_data.decode('utf-8', errors='ignore')
			else:
				js_content = input_data
			
			# Get script directory to find minify_js.js
			script_dir = os.path.dirname(os.path.abspath(__file__))
			minify_script = os.path.join(script_dir, 'minify_js.js')
			
			# Check if Node.js is available
			try:
				# Call Node.js script with JS content via stdin
				process = subprocess.Popen(
					['node', minify_script],
					stdin=subprocess.PIPE,
					stdout=subprocess.PIPE,
					stderr=subprocess.PIPE,
					text=True,
					cwd=script_dir
				)
				stdout, stderr = process.communicate(input=js_content, timeout=30)
				
				if process.returncode == 0:
					if stdout and len(stdout) > 0:
						print("  ✓ Using terser (Node.js)")
						return stdout
					else:
						raise Exception("Node.js minifier returned empty output")
				else:
					raise Exception("Node.js minifier failed (code {}): {}".format(process.returncode, stderr))
			except FileNotFoundError:
				# Fallback to HTTP service if Node.js not available
				print("  ⚠ Node.js not found, falling back to HTTP minifier service")
				data = {'input': js_content}
				response = requests.post(URL_minify_js, data=data, timeout=30)
				return response.text
			except subprocess.TimeoutExpired:
				process.kill()
				raise Exception("Minification timeout")
			except Exception as subprocess_error:
				# Re-raise subprocess errors to be caught by outer handler
				raise Exception("Subprocess error: " + str(subprocess_error))
				
		except Exception as e: 
			print("minify_js FAIL: " + str(e))
			print("minify_js retry " + str(i) + " for " + input_file)
			#time.sleep(1)
			# Retry!
			continue
		break
	print("minify_js error after 10 retries: " + input_file + " using uncompressed data")
	# return uncompressed data (decode bytes to string)
	if isinstance(input_data, bytes):
		return input_data.decode('utf-8', errors='ignore')
	return input_data

def remove_html_comments(html_content):
	"""Remove HTML comments before minification"""
	# Remove <!-- ... --> comments (including multi-line)
	html_content = re.sub(r'<!--.*?-->', '', html_content, flags=re.DOTALL)
	# Remove conditional comments <!--[if ...]--> (IE-specific)
	html_content = re.sub(r'<!--\[if.*?\]>.*?<!\[endif\]-->', '', html_content, flags=re.DOTALL)
	return html_content

def minify_html(input_file):
	"""Minify HTML using html-minifier-terser via Node.js"""
	for i in range(0,10):
		try:
			# Read input file
			input_data = open(input_file, 'rb').read()
			# Decode bytes to string
			if isinstance(input_data, bytes):
				html_content = input_data.decode('utf-8', errors='ignore')
			else:
				html_content = input_data
			
			# Remove comments before minification (Phase 1, Task 1.1)
			html_content = remove_html_comments(html_content)
			
			# Get script directory to find minify_html.js
			script_dir = os.path.dirname(os.path.abspath(__file__))
			minify_script = os.path.join(script_dir, 'minify_html.js')
			
			# Check if Node.js is available
			try:
				# Call Node.js script with HTML content via stdin
				process = subprocess.Popen(
					['node', minify_script],
					stdin=subprocess.PIPE,
					stdout=subprocess.PIPE,
					stderr=subprocess.PIPE,
					text=True,
					cwd=script_dir
				)
				stdout, stderr = process.communicate(input=html_content, timeout=30)
				
				if process.returncode == 0:
					if stdout and len(stdout) > 0:
						print("  ✓ Using html-minifier-terser (Node.js)")
						return stdout
					else:
						raise Exception("Node.js minifier returned empty output")
				else:
					raise Exception("Node.js minifier failed (code {}): {}".format(process.returncode, stderr))
			except FileNotFoundError:
				# Fallback to HTTP service if Node.js not available
				print("  ⚠ Node.js not found, falling back to HTTP minifier service")
				data = {'input': html_content}
				response = requests.post(URL_minify_html, data=data, timeout=30)
				return response.text
			except subprocess.TimeoutExpired:
				process.kill()
				raise Exception("Minification timeout")
			except Exception as subprocess_error:
				# Re-raise subprocess errors to be caught by outer handler
				raise Exception("Subprocess error: " + str(subprocess_error))
				
		except Exception as e: 
			print("minify_html FAIL: " + str(e))
			print("minify_html retry " + str(i) + " for " + input_file)
			#time.sleep(1)
			# Retry!
			continue
		break
	print("minify_html error after 10 retries: " + input_file + " using uncompressed data")
	# return uncompressed data (decode bytes to string)
	if isinstance(input_data, bytes):
		return input_data.decode('utf-8', errors='ignore')
	return input_data

def minify_css(input_file):
	"""Minify CSS using clean-css via Node.js"""
	for i in range(0,10):
		try:
			# Read input file
			input_data = open(input_file, 'rb').read()
			# Decode bytes to string
			if isinstance(input_data, bytes):
				css_content = input_data.decode('utf-8', errors='ignore')
			else:
				css_content = input_data
			
			# Get script directory to find minify_css.js
			script_dir = os.path.dirname(os.path.abspath(__file__))
			minify_script = os.path.join(script_dir, 'minify_css.js')
			
			# Check if Node.js is available
			try:
				# Call Node.js script with CSS content via stdin
				process = subprocess.Popen(
					['node', minify_script],
					stdin=subprocess.PIPE,
					stdout=subprocess.PIPE,
					stderr=subprocess.PIPE,
					text=True,
					cwd=script_dir
				)
				stdout, stderr = process.communicate(input=css_content, timeout=30)
				
				if process.returncode == 0:
					if stdout and len(stdout) > 0:
						print("  ✓ Using clean-css (Node.js)")
						return stdout
					else:
						raise Exception("Node.js minifier returned empty output")
				else:
					raise Exception("Node.js minifier failed (code {}): {}".format(process.returncode, stderr))
			except FileNotFoundError:
				# Fallback to HTTP service if Node.js not available
				print("  ⚠ Node.js not found, falling back to HTTP minifier service")
				data = {'input': css_content}
				response = requests.post(URL_minify_css, data=data, timeout=30)
				return response.text
			except subprocess.TimeoutExpired:
				process.kill()
				raise Exception("Minification timeout")
			except Exception as subprocess_error:
				# Re-raise subprocess errors to be caught by outer handler
				raise Exception("Subprocess error: " + str(subprocess_error))
				
		except Exception as e: 
			print("minify_css FAIL: " + str(e))
			print("minify_css retry " + str(i) + " for " + input_file)
			#time.sleep(1)
			# Retry!
			continue
		break
	print("minify_css error after 10 retries: " + input_file + " using uncompressed data")
	# return uncompressed data (decode bytes to string)
	if isinstance(input_data, bytes):
		return input_data.decode('utf-8', errors='ignore')
	return input_data

def read_file_as_text(fullfilename):
    with open(fullfilename, 'rb') as f:
        data = f.read()
    return data.decode('utf-8', errors='ignore') if isinstance(data, bytes) else data

def process_file(file_symbol_name, filename, fullfilename):
    if filename.endswith(".js"):
        print("Output JAVASRIPT: " + fullfilename)
        if no_minify:
            minified = read_file_as_text(fullfilename)
        else:
            minified = minify_js(fullfilename)          # minify javascript
        hexified = aschii2Hex(minified)                         # convert to hex
    elif filename.endswith(".html"):
        print("Output HTML: " + fullfilename)
        if no_minify:
            minified = read_file_as_text(fullfilename)
        else:
            minified = minify_html(fullfilename)        # minify html
        hexified = aschii2Hex(minified)                         # convert to hex
    elif filename.endswith(".css"):
        print("Output CSS: " + fullfilename)
        if no_minify:
            minified = read_file_as_text(fullfilename)
        else:
            minified = minify_css(fullfilename)         # minify css
        hexified = aschii2Hex(minified)                         # convert to hex
    elif filename.endswith(".png") or filename.endswith(".jpg") or filename.endswith(".gif"):
        print("Output IMAGE: " + fullfilename)
        hexified = file2Hex(fullfilename) # convert file to hex
    else:
        # ignore other files
        print("Ignore File: " + fullfilename)
        return

    # Output to file: file_symbol_name must be a valid C identifier (e.g. images_app_logo_png)
    write_to_file(file_symbol_name, hexified, fullfilename)


for root, dirs, files in os.walk(input_dir, topdown=False):
    # Process Files (fullfilename normalized so output is identical on Windows and Linux)
    for filename in files:
        fullfilename = os.path.join(root, filename).replace(os.sep, '/')
        relpath = fullfilename.replace(input_dir + "/", "").lstrip("/")
        if relpath:
            # Keep the last dot so write_to_file's splitext gets correct name and extension
            file_symbol_name = relpath.replace("/", "_")
        else:
            file_symbol_name = filename
        process_file(file_symbol_name, filename, fullfilename)
 
f_output.close()
