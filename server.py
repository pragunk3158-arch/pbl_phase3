"""
SmartWaste - small web server.

It does two jobs:
  1. serves the files inside the web folder (the dashboard)
  2. runs the compiled C++ program and sends back the JSON it prints

Only the Python standard library is used, so nothing has to be installed.
Run it with:  python server.py
"""

import http.server
import socketserver
import subprocess
import urllib.parse
import os
import sys
import webbrowser

VERSION = 5
FIRST_PORT = 5000
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
WEB_DIR = os.path.join(BASE_DIR, "web")


def find_program():
    """Returns the path of the compiled program, or None if it is missing."""
    for name in ["SmartWaste.exe", "SmartWaste"]:
        path = os.path.join(BASE_DIR, name)
        if os.path.exists(path):
            return path
    return None


PROGRAM = find_program()


def run_program(arguments):
    """
    Runs the C++ program and returns (output, error_message).
    Only one of the two is filled in.

    The timeout and the return code are checked because otherwise a crash
    in the C++ program would send an empty answer to the browser, and the
    page would simply do nothing without telling the user why.
    """
    try:
        result = subprocess.run([PROGRAM] + arguments, capture_output=True,
                                text=True, cwd=BASE_DIR, timeout=15)
    except subprocess.TimeoutExpired:
        return None, "The program took too long and was stopped."
    except OSError as problem:
        return None, "The program could not be started: %s" % problem

    if result.returncode != 0:
        return None, "The program stopped with an error. Check the data files."

    output = result.stdout.strip()
    if output == "":
        return None, "The program did not print anything."

    return output, None


def build_api_arguments(query):
    """Turns the web request into the command line options of the program."""
    def get(name, default=""):
        return query.get(name, [default])[0]

    action = get("action")

    if action == "smart":
        return ["--api", "smart", get("vehicle", "")]
    if action in ("block", "unblock", "reroute"):
        arguments = ["--api", action, get("from"), get("to")]
        if action == "reroute":
            arguments.append(get("reason", "Road blocked"))
        return arguments
    if action == "update":
        return ["--api", "update", get("binId"), get("waste", "0")]
    if action == "addbin":
        return ["--api", "addbin", get("id"), get("location"),
                get("capacity", "100"), get("waste", "0")]
    if action in ("collect", "compare", "reset"):
        return ["--api", action]

    # unknown action: just send the current data back
    return ["--json"]


class ReusableServer(socketserver.TCPServer):
    # On Linux and Mac this lets the server start again right after it was
    # stopped, instead of saying "Address already in use".
    # On Windows the same setting lets TWO servers sit on the same port at
    # once, and then the browser may be talking to an old server that is
    # still running with old code. So it is switched off there.
    allow_reuse_address = (os.name != "nt")


class SmartWasteHandler(http.server.SimpleHTTPRequestHandler):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        query = urllib.parse.parse_qs(parsed.query)

        if parsed.path == "/api/state":
            self.send_program_output(["--json"], "application/json")
        elif parsed.path == "/api/action":
            self.send_program_output(build_api_arguments(query), "application/json")
        elif parsed.path == "/api/test":
            self.send_program_output(["--test"], "text/plain")
        else:
            super().do_GET()          # a normal file from the web folder

    def send_program_output(self, arguments, content_type):
        output, error = run_program(arguments)
        if error is not None:
            print("Error while running the program:", error)
            self.send_text('{"error": "%s"}' % error, "application/json")
        else:
            self.send_text(output, content_type)

    def send_text(self, text, content_type):
        data = text.encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def end_headers(self):
        # The browser keeps a copy of index.html, style.css and app.js and
        # reuses it. After the files are changed that old copy does not match
        # the new data any more and the page stops working. This header tells
        # the browser to always ask the server again.
        self.send_header("Cache-Control", "no-store, must-revalidate")
        super().end_headers()

    def log_message(self, format, *args):
        pass                          # keeps the console clean


def main():
    if PROGRAM is None:
        print("ERROR: the compiled program was not found.")
        print("Compile it first: run build.bat on Windows, or 'make' on Linux.")
        print("If build.bat fails, g++ (MinGW) is probably not installed.")
        input("Press Enter to close...")
        sys.exit(1)

    if not os.path.isdir(WEB_DIR):
        print("ERROR: the web folder is missing next to server.py.")
        input("Press Enter to close...")
        sys.exit(1)

    # A quick check that the program really runs before the browser opens
    output, error = run_program(["--json"])
    if error is not None:
        print("ERROR: the program does not run properly:", error)
        input("Press Enter to close...")
        sys.exit(1)

    # If an older server is still running on port 5000 the browser would talk
    # to that old one instead of this one, and the page would look broken.
    # So a free port is looked for and the browser is sent to that port.
    server = None
    port = FIRST_PORT
    while port < FIRST_PORT + 10:
        try:
            server = ReusableServer(("", port), SmartWasteHandler)
            break
        except OSError:
            print("Port %d is busy, trying %d ..." % (port, port + 1))
            port = port + 1

    if server is None:
        print("ERROR: no free port between %d and %d." % (FIRST_PORT, FIRST_PORT + 9))
        input("Press Enter to close...")
        sys.exit(1)

    address = "http://localhost:%d" % port

    print("====================================================")
    print("  SmartWaste dashboard  (version %d)" % VERSION)
    print("  Open %s in your browser" % address)
    print("  Press Ctrl+C to stop")
    print("====================================================")

    try:
        # the number at the end makes the browser fetch a fresh page
        # instead of showing the copy it saved earlier
        webbrowser.open(address + "/?v=%d" % VERSION)
    except Exception:
        pass

    with server:
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")


if __name__ == "__main__":
    main()
