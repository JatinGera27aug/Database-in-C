from cache22_client import Cache22Client

def prompt(prompt_text, allow_empty=False):
    while True:
        v = input(prompt_text).strip()
        if v or allow_empty:
            return v

def main():
    host = input("Host [127.0.0.1]: ").strip() or "127.0.0.1"
    port = input("Port [8080]: ").strip() or "8080"

    client = Cache22Client(host, port)
    try:
        client.connect()
    except Exception as e:
        print("Failed to connect:", e)
        return

    print("Connected. Commands: add, serve, del, flush, top, exit")
    while True:
        cmd = input("> ").strip().lower()
        if not cmd:
            continue
        if cmd in ("exit", "quit"):
            break

        if cmd == "add":
            key = prompt("Key: ")
            value = prompt("Value: ")
            ttl = prompt("TTL seconds (enter for default 120): ", allow_empty=True)
            if ttl:
                resp = client.send_command(f"add {key} {value} -ttl {ttl}")
            else:
                resp = client.send_command(f"add {key} {value}")
            print(f"Server reply: {resp}")

        elif cmd == "serve":
            key = prompt("Key: ")
            resp = client.send_command(f"serve {key}")
            if not resp:
                print("Result: (no value)")
            elif resp.upper().startswith("ERR") or resp.upper().startswith("NO"):
                print("Result: (server returned error / no value)")
                print("Server reply:", resp)
            else:
                print("Value:", resp.splitlines()[0])

        elif cmd == "del":
            key = prompt("Key: ")
            resp = client.send_command(f"del {key}")
            print("Server reply:", resp)

        elif cmd == "flush":
            resp = client.send_command("flush")
            print("Server reply:", resp)

        elif cmd == "top":
            n = prompt("Top N (enter for 5): ", allow_empty=True) or "5"
            resp = client.send_command(f"top {n}")
            if not resp or resp.upper().startswith("OK (NO") or resp.upper().startswith("OK (NO"):
                print("No keys to show.")
            else:
                print("Top results:")
                for line in resp.splitlines():
                    print(" ", line)
        else:
            print("Unknown command")

    client.close()
    print("Disconnected.")

if __name__ == "__main__":
    main()