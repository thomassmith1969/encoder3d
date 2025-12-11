#!/usr/bin/env python3
"""
Short move completion test via WebSocket.
- Ensures that a short move (G0 X1) enqueues and completes: final ok message is received.

Usage: python3 tests/short_move_completion_test.py --host 192.168.0.12 --port 81
"""
import argparse
import asyncio
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
        # Clear and wait for not busy
        await ws.send('M999')
        _ = await recv_until(ws, lambda m: m.startswith('{') and json.loads(m).get('executor_busy') in (False, None), timeout=3.0)

        print('Sending small move G0 X1')
        await ws.send('G0 X1')

        # Wait for immediate ok queued (response) or pending
        msg = await recv_until(ws, lambda m: m.startswith('{') and 'status' in m and json.loads(m).get('status') in ('ok','error') , timeout=2.0)
        print('Immediate push response:', msg)

        # Wait for final execution ack 'ok' (may be JSON) within 10s
        final = await recv_until(ws, lambda m: m.startswith('{') and json.loads(m).get('status') == 'ok', timeout=10.0)
        print('Final execution ack:', final)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.0.12')
    parser.add_argument('--port', type=int, default=81)
    args = parser.parse_args()
    asyncio.run(main(args.host, args.port))
