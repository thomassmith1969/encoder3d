#!/usr/bin/env python3
"""
WebSocket test harness for encoder3d.

This script connects two websocket clients to the device (owner and other) and
verifies push-time behavior under three scenarios:
 1) Immediate enqueue when the queue has space -> ok:queued
 2) Non-owner push while executor busy -> immediate error:busy
 3) Owner push when queue is full -> no immediate reply (pending), then final
    ok:queued or error:busy from the background waiter.

Usage: python3 tests/ws_test.py --host 192.168.4.1 --port 81

Requires: websockets (pip install websockets)
"""
import asyncio
import argparse
import json
import time

import websockets


async def recv_until(ws, match_fn, timeout=5.0):
    """Receive messages until match_fn(msg) returns True or timeout.
    Returns the matching message or None on timeout.
    """
    start = time.time()
    while time.time() - start < timeout:
        try:
            msg = await asyncio.wait_for(ws.recv(), timeout=timeout - (time.time() - start))
        except Exception:
            return None
        if match_fn(msg):
            return msg
    return None


async def main(host, port):
    uri = f"ws://{host}:{port}"
    print(f"Connecting two websocket clients to {uri}")

    async with websockets.connect(uri) as ws_owner, websockets.connect(uri) as ws_other:
        print("Connected both clients")

        # Subscribe that we'll listen for broadcasts as they arrive.
        # Send a long-running-ish command from owner to claim executor.
        print("-> Owner sending long move to claim executor (G0 X100000)")
        await ws_owner.send("G0 X100000")

        # Wait for immediate ack ok:queued from owner
        msg = await recv_until(ws_owner, lambda m: m.startswith('ok:') or m.startswith('error:'), timeout=2.0)
        print("Owner immediate response:", msg)

        # Wait until broadcast reports executor_busy true (status JSON messages are broadcast)
        print("Waiting for executor to be busy in status broadcasts...")
        status_msg = await recv_until(ws_owner, lambda m: m.startswith('{') and 'executor_busy' in m and json.loads(m).get('executor_busy') is True, timeout=5.0)
        print("Got status broadcast showing executor busy:", status_msg)

        # Non-owner push while busy -> should get immediate error:busy
        print("-> Other client pushing while executor busy -> expect immediate error:busy")
        await ws_other.send("G0 X1")
        resp = await recv_until(ws_other, lambda m: m.startswith('error:') or m.startswith('ok:'), timeout=1.0)
        print("Other client response:", resp)

        # Now flood many commands from owner to fill commandQueue.
        # The server's commandQueue default size is 32 — we'll send 64 pushes quickly.
        print("-> Flooding owner with many commands (64) to fill the commandQueue")
        pending_indices = []
        immediate_ok_count = 0
        for i in range(64):
            await ws_owner.send(f"G0 X{i}")
            # Try to get immediate response within 50ms
            msg = await recv_until(ws_owner, lambda m: m.startswith('ok:') or m.startswith('error:'), timeout=0.05)
            if msg is None:
                # No immediate response = 'pending' behavior (background waiter will reply later)
                pending_indices.append(i)
            else:
                if msg.startswith('ok:'):
                    immediate_ok_count += 1
        print(f"Immediate ok: {immediate_ok_count}, pending (no immediate reply): {len(pending_indices)}")

        # For all pending pushes, wait for final ack (ok:queued or error:busy) within 5s each
        final_ok = 0
        final_busy = 0
        start = time.time()
        while time.time() - start < 10 and (len(pending_indices) > 0):
            try:
                msg = await asyncio.wait_for(ws_owner.recv(), timeout=5.0)
            except Exception:
                break
            if msg.startswith('ok:queued'):
                final_ok += 1
                pending_indices.pop(0)
            elif msg.startswith('error:busy'):
                final_busy += 1
                pending_indices.pop(0)
            else:
                # Could be status or other messages
                pass
        print(f"Final ack results for pending: ok={final_ok}, busy={final_busy}, leftover_pending={len(pending_indices)}")

        # Done
        print("Test complete")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.4.1', help='Device hostname or IP')
    parser.add_argument('--port', type=int, default=81)
    args = parser.parse_args()

    asyncio.run(main(args.host, args.port))
