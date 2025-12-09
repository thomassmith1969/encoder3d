#!/usr/bin/env python3
"""
Emergency and Clear tests for encoder3d over WebSocket.
- Send M112 (emergency) and expect final ok:emergency (JSON response) and that the system reports halted.
- Send M999 (clear) and expect the system clears the halt state in status broadcasts.

Usage: python3 tests/emergency_test.py --host 192.168.0.12 --port 81
"""
import asyncio
import argparse
import json
import time

import websockets


async def recv_until(ws, condition, timeout=5.0):
    start = time.time()
    while time.time() - start < timeout:
        try:
            msg = await asyncio.wait_for(ws.recv(), timeout=timeout - (time.time() - start))
        except Exception:
            return None
        if condition(msg):
            return msg
    return None


async def main(host, port):
    uri = f"ws://{host}:{port}"
    print(f"Connecting websocket to {uri}")

    async with websockets.connect(uri) as ws:
        print("Connected")

        # Clear first, wait for clear to be visible in status
        await ws.send("M999")
        # Wait for a status broadcast where there is no 'error' and executor_busy is False
        _ = await recv_until(ws, lambda m: m.startswith('{') and (json.loads(m).get('executor_busy') is False), timeout=3.0)

        # Send M112 - emergency
        print('-> Sending M112 (emergency)')
        await ws.send('M112')

        # We expect the controlTask to send a final ok:emergency JSON response
        msg = await recv_until(ws, lambda m: m.startswith('{') and 'status' in m and json.loads(m).get('status') in ('ok','error'), timeout=5.0)
        print('M112 response:', msg)

        # Wait for broadcast that shows system is halted or for an error broadcast (JSON) with halt reason
        halted_msg = await recv_until(ws, lambda m: m.startswith('{') and json.loads(m).get('executor_busy') in (True, False) and ('error' in m or json.loads(m).get('executor_busy')==True), timeout=5.0)
        print('Status/Halt broadcast (post M112):', halted_msg)

        # Clear with M999
        print('-> Sending M999 (clear)')
        await ws.send('M999')
        # Wait for status broadcast where there is no error/halt indicated
        clear_ok = await recv_until(ws, lambda m: m.startswith('{') and 'error' not in m and json.loads(m).get('executor_busy') in (True, False), timeout=5.0)
        print('Status after clear:', clear_ok)

        print('Emergency test completed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.0.12')
    parser.add_argument('--port', type=int, default=81)
    args = parser.parse_args()
    asyncio.run(main(args.host, args.port))
