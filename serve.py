# serve.py
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import mimetypes
import os

mimetypes.add_type('application/wasm', '.wasm')

class COOPCOEPHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        super().end_headers()

if __name__ == "__main__":
    os.chdir("public")
    server = ThreadingHTTPServer(("127.0.0.1", 6969), COOPCOEPHandler)
    print("Serving on http://127.0.0.1:6969 (COOP+COEP enabled)")
    server.serve_forever()
