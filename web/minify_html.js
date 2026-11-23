#!/usr/bin/env node
/**
 * HTML Minifier Wrapper using html-minifier-terser
 * Called from convert.py with HTML content via stdin
 * Outputs minified HTML to stdout
 */

const minify = require('html-minifier-terser').minify;
const fs = require('fs');

// Read HTML from stdin
let html = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data', (chunk) => {
  html += chunk;
});

process.stdin.on('end', async () => {
  try {
    // Aggressive minification options
    const options = {
      collapseWhitespace: true,
      removeComments: true,              // Remove HTML comments
      removeOptionalTags: true,           // Remove </li>, </p>, etc. where HTML5 allows
      removeRedundantAttributes: true,    // Remove type="text", etc.
      removeScriptTypeAttributes: true,   // Remove type="text/javascript"
      removeStyleLinkTypeAttributes: true, // Remove type="text/css"
      minifyCSS: true,                    // Minify inline <style> blocks
      minifyJS: true,                     // Minify inline <script> blocks
      quoteCharacter: '"',
      caseSensitive: false,
      collapseBooleanAttributes: true,    // disabled="disabled" → disabled
      removeEmptyAttributes: true,
      removeEmptyElements: false,        // Keep false to avoid breaking structure
      removeAttributeQuotes: false,      // Keep false for compatibility
      useShortDoctype: true,             // <!DOCTYPE html> instead of full doctype
      minifyURLs: false,                 // Keep URLs intact
      keepClosingSlash: false,           // Remove closing slashes from void elements
      sortAttributes: false,             // Don't sort (may break jQuery Mobile)
      sortClassName: false               // Don't sort classes
    };

    // minify() returns a Promise in version 7+
    const minified = await minify(html, options);
    process.stdout.write(minified);
  } catch (error) {
    console.error('Error minifying HTML:', error.message);
    process.exit(1);
  }
});
