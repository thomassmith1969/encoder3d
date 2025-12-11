#!/usr/bin/env python3
"""
Telnet test harness for encoder3d.

This script uses a websocket client as owner and a telnet client as the second source.
It verifies:
 - non-owner receives immediate error:busy while executor is busy
 - owner can flood commands and pending/ok behavior is observed

Usage: python3 tests/telnet_test.py --host 192.168.4.1 --ws-port 81 --telnet-port 23

Requires: websockets (pip install websockets)
"""
import argparse
import asyncio
import json
import time
import socket

import websockets


async def recv_until(ws, match_fn, timeout=5.0):
    start = time.time()
    while time.time() - start < timeout:
        try:
            msg = await asyncio.wait_for(ws.recv(), timeout=timeout - (time.time() - start))
        except Exception:
            return None
        if match_fn(msg):
            return msg
    return None


async def main(host, ws_port, telnet_port):
    ws_uri = f"ws://{host}:{ws_port}"
    print(f"Connecting owner websocket to {ws_uri}")

    async with websockets.connect(ws_uri) as ws_owner:
        print("Connected owner ws")
        # Claim executor by sending long move
        print("Owner sending long move (G0 X100000)")
        await ws_owner.send("G0 X100000")
        msg = await recv_until(ws_owner, lambda m: (m.startswith('{') and 'status' in m) or m.startswith('ok:') or m.startswith('error:'), timeout=2.0)
        print("Owner immediate response:", msg)

        # Wait until executor busy broadcast
        status_msg = await recv_until(ws_owner, lambda m: m.startswith('{') and 'executor_busy' in m and json.loads(m).get('executor_busy') is True, timeout=5.0)
        print("Executor busy status seen:" , status_msg)

        # Connect telnet and attempt a push while executor is busy
        print(f"Connecting telnet to {host}:{telnet_port}")
        tn = socket.create_connection((host, telnet_port), timeout=5)
        tn.settimeout(2.0)
        # Flush greeting
        try:
            time.sleep(0.1)
            out = tn.recv(1024).decode('utf-8', errors='ignore')
            if out:
                print('Telnet greeting:', out.strip())
        except Exception:
            pass

        print("Telnet client sending G0 X1 (expect immediate error:busy)")
        tn.sendall(b"G0 X1\n")
        # Wait for response line
        try:
            resp = tn.recv(1024)
            print('Telnet response:', resp.decode('utf-8', errors='ignore').strip())
        except Exception:
            print('Telnet response: <no response or timeout>')

        # Now flood owner commands to try fill queue
        print('Flooding owner (64 pushes)')
        pending = []
        immediate_ok = 0
        for i in range(64):
            await ws_owner.send(f"G0 X{i}")
            msg = await recv_until(ws_owner, lambda m: (m.startswith('{') and 'status' in m) or m.startswith('ok:') or m.startswith('error:'), timeout=0.05)
            if msg is None:
                pending.append(i)
            else:
                if msg.startswith('{'):
                    parsed = json.loads(msg)
                    if parsed.get('status') == 'ok': immediate_ok += 1
                elif msg.startswith('ok:'):
                    immediate_ok += 1
        print('immediate_ok=', immediate_ok, 'pending=', len(pending))

        # Wait for final ack for pending
        final_ok = 0
        final_busy = 0
        start = time.time()
        while time.time() - start < 10 and pending:
            try:
                msg = await asyncio.wait_for(ws_owner.recv(), timeout=5.0)
            except Exception:
                break
            if msg.startswith('{'):
                parsed = json.loads(msg)
                if parsed.get('status') == 'ok': final_ok += 1; pending.pop(0)
                elif parsed.get('status') == 'error': final_busy += 1; pending.pop(0)
            else:
                if msg.startswith('ok:queued'):
                    final_ok += 1; pending.pop(0)
                elif msg.startswith('error:busy'):
                    final_busy += 1; pending.pop(0)
        print('final_ok=', final_ok, 'final_busy=', final_busy, 'leftover_pending=', len(pending))

        tn.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.4.1')
    parser.add_argument('--ws-port', type=int, default=81)
    parser.add_argument('--telnet-port', type=int, default=23)
    args = parser.parse_args()

    asyncio.run(main(args.host, args.ws_port, args.telnet_port))
