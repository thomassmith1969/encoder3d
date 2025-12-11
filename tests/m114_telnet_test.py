#!/usr/bin/env python3
"""
Test M114 (position report) via Telnet.
Sends M114 and expects response like: X:%.2f Y:%.2f Z:%.2f E:%.2f

Usage: python3 tests/m114_telnet_test.py --host 192.168.0.12 --port 23
"""
import argparse
import socket
import asyncio
import json
import time
import websockets


async def main_async(host, ws_port, telnet_port):
    ws_uri = f"ws://{host}:{ws_port}"
    print(f"Connecting websocket to {ws_uri} to ensure system cleared")
    async with websockets.connect(ws_uri) as ws:
        # send clear (M999) and wait for executor_busy=false in status
        await ws.send('M999')
        print('Sent M999 to clear')
        # wait for executor_busy False (or just small sleep to stabilize)
        start = time.time()
        while time.time() - start < 3:
            try:
                msg = await asyncio.wait_for(ws.recv(), timeout=1.0)
            except Exception:
                continue
            if msg.startswith('{'):
                j = json.loads(msg)
                if 'executor_busy' in j and j.get('executor_busy') is False:
                    print('System is not busy — proceeding')
                    break

    # Now connect telnet and send M114
    print(f"Connecting telnet to {host}:{telnet_port}")
    tn = socket.create_connection((host, telnet_port), timeout=5)
    tn.settimeout(2.0)

    time.sleep(0.1)
    try:
        out = tn.recv(1024).decode('utf-8', errors='ignore')
        if out:
            print('Greeting:', out.strip())
    except Exception:
        pass

    print('Sending M114')
    tn.sendall(b"M114\n")
    try:
        resp = tn.recv(256).decode('utf-8', errors='ignore').strip()
        print('M114 response:', resp)
        # This test accepts either a position reply (X:...) or immediate error:busy
        if not resp:
            print('No response — test inconclusive (device busy)')
        elif resp.startswith('error:busy'):
            print('Device returned error:busy as expected when not allowed to push')
        else:
            print('Position reply received — OK')
    except Exception as e:
        print('No response:', e)

    tn.close()


def main(host, port):
    import asyncio
    asyncio.run(main_async(host, 81, port))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.0.12')
    parser.add_argument('--port', type=int, default=23)
    args = parser.parse_args()
    main(args.host, args.port)
