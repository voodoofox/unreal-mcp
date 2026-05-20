"""Test 3 parallel TCP connections to the Unreal MCP server."""
import socket
import json
import threading
import time

HOST = "127.0.0.1"
PORT = 55557

def send_command(label, command, params):
    """Send a command on a fresh TCP connection and return result."""
    t0 = time.perf_counter()
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.connect((HOST, PORT))
        elapsed_connect = time.perf_counter() - t0

        msg = json.dumps({"type": command, "params": params}, separators=(",", ":")) + "\n"
        sock.sendall(msg.encode("utf-8"))

        buf = b""
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                raise Exception("Connection closed")
            buf += chunk
            if b"\n" in buf:
                break

        elapsed_total = time.perf_counter() - t0
        resp = json.loads(buf[:buf.index(b"\n")].decode("utf-8"))
        status = resp.get("status", "?")
        print(f"[{label}] OK  connect={elapsed_connect:.3f}s  total={elapsed_total:.3f}s  status={status}")
        return resp
    except Exception as e:
        elapsed_total = time.perf_counter() - t0
        print(f"[{label}] FAIL  total={elapsed_total:.3f}s  error={e}")
        return None
    finally:
        try:
            sock.close()
        except:
            pass

commands = [
    ("cmd1", "run_console_command", {"command": "stat unit"}),
    ("cmd2", "find_actors_by_name", {"pattern": "PlayerStart"}),
    ("cmd3", "get_material_info",   {"asset_path": "/Engine/BasicShapes/BasicShapeMaterial"}),
]

print(f"Launching {len(commands)} parallel TCP connections to {HOST}:{PORT}...")
t_start = time.perf_counter()

threads = []
for label, cmd, params in commands:
    t = threading.Thread(target=send_command, args=(label, cmd, params))
    threads.append(t)

for t in threads:
    t.start()
for t in threads:
    t.join(timeout=15)

print(f"\nAll done in {time.perf_counter() - t_start:.3f}s")
