Test scripts for encoder3d

These scripts exercise the live device over WebSocket and Telnet to verify push semantics:

Prerequisites
- Python 3.8+
- pip packages: websockets

Install:

    pip install websockets

Usage (examples)
- WebSocket test (default host 192.168.4.1):

    python3 tests/ws_test.py --host 192.168.4.1 --port 81

- Telnet test (owner via websocket, telnet as other):

    python3 tests/telnet_test.py --host 192.168.4.1 --ws-port 81 --telnet-port 23

What these tests verify
- Immediate ok:queued when commandQueue has space
- Immediate error:busy for non-owner pushes while executor is busy
- For owner pushes when commandQueue is full, network thread does not block — tests will observe no immediate reply and later ok:queued or error:busy from the background waiter

Notes
- The tests assume the device is reachable at the supplied host and ports and that the firmware currently running is the workspace firmware (which broadcasts status and runs background waiter logic).
- Because these are live integration tests, results may vary on real hardware timing. The scripts use short timeouts to detect immediate vs delayed replies. Adjust timeouts if required for your environment.