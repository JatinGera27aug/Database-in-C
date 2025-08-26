import socket
# from contextlib import closing

class Cache22Client:
    """client for TCP server."""

    def __init__(self, host='127.0.0.1', port=8080, recv_timeout=0.5):
        self.host = host
        self.port = int(port)
        self.sock = None
        self.recv_timeout = recv_timeout

    def connect(self):
        if self.sock:
            return
        self.sock = socket.create_connection((self.host, self.port), timeout=2)
        self.sock.settimeout(self.recv_timeout)
        self._recv_all()

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            finally:
                self.sock = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()

    def _send(self, line):
        if not self.sock:
            raise RuntimeError("Not connected")
        data = (line.rstrip("\r\n") + "\r\n").encode('utf-8')
        self.sock.sendall(data)

    def _recv_all(self):
        if not self.sock:
            return ''
        pieces = []
        try:
            while True:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                pieces.append(chunk)
        except socket.timeout:
            pass
        except OSError:
            pass
        return b''.join(pieces).decode('utf-8', errors='replace')

    def send_command(self, line):
        """Send raw command and return server reply (string)."""
        self._send(line)
        return self._recv_all().strip()

    def add(self, key, value, ttl=None):
        if ttl is None:
            cmd = f"add {key} {value}"
        else:
            cmd = f"add {key} {value} -ttl {int(ttl)}"
        return self.send_command(cmd)

    def serve(self, key):
        """Return value string or None if not found."""
        resp = self.send_command(f"serve {key}")
        if not resp:
            return None
        # server may return ERR messages; treat lines starting with ERR as None
        if resp.upper().startswith("ERR") or resp.upper().startswith("NO"):
            return None
        return resp.splitlines()[0]

    def delete(self, key):
        resp = self.send_command(f"del {key}")
        return resp.upper().startswith("OK")

    def flush(self):
        resp = self.send_command("flush")
        return resp.upper().startswith("OK")

    def top(self, n=5):
        """Return list of (key, count) tuples."""
        resp = self.send_command(f"top {int(n)}")
        out = []
        for line in resp.splitlines():
            parts = line.strip().split()
            if not parts:
                continue
            key = parts[0]
            cnt = 0
            if len(parts) > 1:
                try:
                    cnt = int(parts[1])
                except ValueError:
                    cnt = 0
            out.append((key, cnt))
        return out


def connect(host='127.0.0.1', port=8080):
    c = Cache22Client(host, port)
    c.connect()
    return c