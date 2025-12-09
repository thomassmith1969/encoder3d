#!/usr/bin/env python3
"""
Verify lowercase g-code via WebSocket is parsed and executed.
"""
import asyncio
import argparse
import json
import time
import websockets


async def recv_until(ws, cond, timeout=5.0):
    start = time.time()
    while time.time() - start < timeout:
        try:
            msg = await asyncio.wait_for(ws.recv(), timeout=timeout - (time.time() - start))
        except Exception:
            return None
        if cond(msg):
            return msg
    return None


async def main(host, port):
    uri = f"ws://{host}:{port}"
    print('Connecting to', uri)
    async with websockets.connect(uri) as ws:
        # ensure device is cleared
        await ws.send('M999')
        await asyncio.sleep(0.1)

        print('Sending lowercase move: "G0 x1 y1"')
        await ws.send('G0 x1 y1')
        # immediate response (JSON) expected
        msg = await recv_until(ws, lambda m: m.startswith('{') and 'status' in m, timeout=2.0)
        print('Immediate response:', msg)

        # Wait for final execution ack (ok) within 5s
        final = await recv_until(ws, lambda m: m.startswith('{') and json.loads(m).get('status') == 'ok', timeout=5.0)
        print('Final execution ack (ok):', final)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.0.12')
    parser.add_argument('--port', type=int, default=81)
    args = parser.parse_args()
    asyncio.run(main(args.host, args.port))
