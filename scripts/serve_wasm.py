#!/usr/bin/env python3
"""Tiny dev server for Earthcall WASM with correct MIME types."""
import http.server
import sys
import os

class WasmHandler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        '.wasm': 'application/wasm',
        '.js':   'application/javascript',
    }

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    directory = sys.argv[2] if len(sys.argv) > 2 else 'build-wasm'
    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', directory))
    print(f"Serving {os.getcwd()} on http://localhost:{port}")
    http.server.HTTPServer(('', port), WasmHandler).serve_forever()
