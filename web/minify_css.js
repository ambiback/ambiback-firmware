#!/usr/bin/env node
/**
 * CSS Minifier Wrapper using clean-css
 * Called from convert.py with CSS content via stdin
 * Outputs minified CSS to stdout
 */

const CleanCSS = require('clean-css');

// Read CSS from stdin
let css = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data', (chunk) => {
  css += chunk;
});

process.stdin.on('end', () => {
  try {
    // CleanCSS minification options
    const options = {
      level: {
        1: {
          specialComments: 0,       // Remove all comments
          removeEmpty: true,        // Remove empty rules
          removeWhitespace: true    // Remove whitespace
        },
        2: {
          mergeMedia: true,         // Merge @media rules
          removeUnusedAtRules: false, // Keep @rules
          restructureRules: false   // Don't restructure (safer)
        }
      },
      compatibility: {
        '*': {
          colors: {
            hexAlpha: false         // Keep hex colors
          }
        }
      },
      format: false                 // Don't format output
    };

    const output = new CleanCSS(options).minify(css);
    
    if (output.errors && output.errors.length > 0) {
      console.error('CSS minification warnings:', output.errors.join(', '));
      // Continue anyway - output may still be valid
    }
    
    if (output.styles) {
      process.stdout.write(output.styles);
    } else {
      console.error('Error: CleanCSS returned no output');
      process.exit(1);
    }
  } catch (error) {
    console.error('Error minifying CSS:', error.message);
    process.exit(1);
  }
});
