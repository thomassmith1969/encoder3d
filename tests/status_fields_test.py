#!/usr/bin/env python3
"""
Status broadcast test: Connect via WebSocket and verify status JSON contains expected fields.

Verifies presence of: x,y,z,e, executor_busy, executor_owner_type, executor_owner_id
"""
import asyncio
import argparse
import json
import time

import websockets


async def main(host, port):
    uri = f"ws://{host}:{port}"
    print('Connecting to', uri)
    async with websockets.connect(uri) as ws:
        print('Connected')
        # Wait for a status broadcast that contains the keys
        def check(m):
            if not m.startswith('{'): return False
            try:
                j = json.loads(m)
            except Exception:
                return False
            for k in ('x','y','z','e','executor_busy','executor_owner_type','executor_owner_id'):
                if k not in j:
                    return False
            return True

        msg = None
        for _ in range(30):
            try:
                msg = await asyncio.wait_for(ws.recv(), timeout=1.0)
            except Exception:
                continue
            if check(msg):
                print('Got status message:', msg)
                print('Status fields verified')
                return
        print('Failed to receive valid status JSON in time')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.0.12')
    parser.add_argument('--port', type=int, default=81)
    args = parser.parse_args()
    asyncio.run(main(args.host, args.port))
