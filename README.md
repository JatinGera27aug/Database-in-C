# cacheBase — lightweight Redis-like cache (C, Windows)

Overview
--------
cacheBase is a small TCP key-value cache server implemented in C (Winsock/Win32) with:
- per-client isolated caches (each client connection gets its own HashTable),
- TTL support (default 120s, optional per-key TTL),
- lazy expiry (keys removed on access when expired),
- basic Redis-like commands: add, serve, del, flush, top,
- a Win32 GUI client (C) and a tiny Python client + examples for integration.

This repo is a demo / teaching project to show how to build a simple networked cache and how clients can use it.

Features
--------
- Commands
  - add <key> <value> [-ttl N]  — insert key with optional TTL (seconds). Default TTL = 120s.
  - serve <key>                — return value or error if not found / expired.
  - del <key>                  — delete single key.
  - flush                      — delete all keys for this client (per-client isolation).
  - top [N]                    — return top-N most frequently served keys (access count).
- TTL behavior
  - Lazy expiry: insertion stores expiry timestamp (time(NULL)+ttl). Expired entries are removed when their bucket is accessed (get/insert/delete/scan).
- Per-client isolation
  - Each connection gets its own HashTable; keys are visible only to that connection.
- Statistics
  - Per-key access_count incremented on successful serve; used by top command.

Project layout
--------------
- cache22/ (main working folder)
  - cache22.c, cache22.h        — server logic, request parsing, handlers
  - hash_table.c, hash_table.h  — hash table implementation, TTL, top-N, destroy, delete
  - tree.c, tree.h              — small utility included in the build (not core to cache22)
  - cache22_gui.c               — Win32 GUI client (single-file C)
  - cache22_client.py           — tiny Python wrapper client (send commands, parse replies)
  - interactive_client.py       — interactive CLI client (Python) for demos
  - example_integration.py      — Python example showing caching DB calls using cache22
  - Makefile / build scripts    — builds server and utilities with mingw

###Key implementation notes
------------------------
- Hash table (hash_table.c/h)
  - HashEntry { key, value, time_t expire, next, access_count }
  - insert_key_value(table, key, value, ttl) — sets expire = time(NULL)+ttl (0 means no expiry)
  - get_key_value(table, key) — removes expired nodes on scan and increments access_count for hits
  - delete_key_value(), destroy_hash_table()
  - get_top_n(table, n, out, out_len) — scans table and returns top-N by access_count (simple selection)

- Server (cache22.c / cache22.h)
  - Each accepted connection gets a Client struct with its own HashTable*
  - Handlers call insert/get/delete on client->cache
  - Commands are parsed from line input and routed to handler functions
  - Child loop runs per connection (threaded). Since client cache is single-threaded per connection, no locks required for current design.

Build prerequisites (Windows)
-----------------------------
- mingw/msys or equivalent GCC for Windows (gcc, mingw32-make)
- Python 3.x (for Python client/examples)
- Optional: an editor / Visual Studio Code

Build server and libs
---------------------
From the `cache22` folder:

- Build with the included makefile:
  mingw32-make

This builds:
- cache22.exe       — the server
- tree.exe          — small included utility

If you prefer manual compile:
gcc -O2 -Wall -std=c11 -c hash_table.c
gcc -O2 -Wall -std=c11 -c cache22.c
gcc -O2 -Wall -std=c11 hash_table.o cache22.o -o cache22.exe -lws2_32

Run server
----------
1. Start server:
   cache22.exe
   (By default listens on 127.0.0.1:8080 — see cache22.h to change)

2. Use telnet or a client to interact:
   telnet 127.0.0.1 8080

Protocol examples (line-based, CRLF terminated)
- add foo bar
  -> OK
- add foo jam -ttl 30
  -> OK
- serve foo
  -> jam   (or ERR key not found)
- del foo
  -> OK or ERR key not found
- flush
  -> OK
- top 5
  -> lines like: key count

Win32 GUI client (C)
--------------------
A single-file GUI client is provided (cache22_gui.c).

Build:
gcc cache22_gui.c -o cache22_gui.exe -mwindows -lws2_32

Run: double-click cache22_gui.exe or run from command line.  
UI: Host/Port fields, Key/Value/TTL inputs and buttons for Add / Serve / Del / Flush / Top.

Python client & examples
------------------------
The Python wrapper (cache22_client.py) provides a tiny programmatic client.

Files:
- cache22_client.py   — Cache22Client class: connect(), add(), serve(), delete(), flush(), top()
- interactive_client.py — interactive prompt to run commands


Usage:
1. Ensure server is running.
2. Run interactive demo:
   python interactive_client.py


Notes on the Python client:
- Uses simple line-based protocol. It uses a short recv timeout to gather replies. For production, consider a framed protocol or RESP.


Limitations and next steps
--------------------------
- Lazy expiry only removes expired keys when buckets are touched. Consider adding a background periodic cleaner thread to aggressively free expired entries (requires synchronization).
- No authentication, TLS, persistence (AOF or RDB). Add if durability/security is required.
- Protocol: currently line-based. For robust clients consider implementing RESP (Redis protocol) or length-prefixed framing.
- top-N is simple scan; for large tables consider a dedicated min-heap or space-saving algorithm.
- Per-client isolation is currently implemented. If you want shared global cache, move to a shared HashTable with locks.

Recommended quick roadmap to make it "Redis-like" for a project
1. Stabilize protocol and document it (this README covers the basics).
2. Provide stable client libraries (we have Python; add more if needed).
3. Add optional persistence (RDB/AOF) if you need restart recovery.
4. Harden concurrency if moving to a global cache with multiple threads.
5. Add auth/TLS if deployed to untrusted networks.

Contact / usage
---------------
- To demo: run server, open two interactive clients (telnet or interactive_client.py) and try per-client isolation (each client sees its own keys).
- To use in a Python project: import Cache22Client from cache22_client.py and call add/serve/delete/flush/top.
