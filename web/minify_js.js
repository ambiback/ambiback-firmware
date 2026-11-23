#!/usr/bin/env node
/**
 * JavaScript Minifier Wrapper using terser
 * Called from convert.py with JS content via stdin
 * Outputs minified JS to stdout
 */

const { minify } = require('terser');

// Read JavaScript from stdin
let js = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data', (chunk) => {
  js += chunk;
});

process.stdin.on('end', async () => {
  try {
    // Terser minification options
    const options = {
      compress: {
        drop_console: false,        // Keep console statements
        drop_debugger: true,        // Remove debugger statements
        pure_funcs: null,           // Don't remove specific functions
        passes: 1                   // Number of compression passes
      },
      mangle: {
        toplevel: false,            // Don't mangle top-level names
        keep_classnames: false,      // Mangle class names
        keep_fnames: false          // Mangle function names
      },
      format: {
        comments: false,            // Remove comments
        beautify: false,            // Don't beautify output
        preserve_annotations: false
      },
      parse: {
        ecma: 5                     // Support ES5
      }
    };

    // minify() returns a Promise
    const result = await minify(js, options);
    
    if (result.error) {
      console.error('Error minifying JavaScript:', result.error.message);
      process.exit(1);
    }
    
    if (result.code) {
      process.stdout.write(result.code);
    } else {
      console.error('Error: Terser returned no output');
      process.exit(1);
    }
  } catch (error) {
    console.error('Error minifying JavaScript:', error.message);
    process.exit(1);
  }
});
