#!/usr/bin/env python3
"""
End-to-end movement test (WebSocket):
- Ensures that a small move command (G0 X<n>) both gets queued, and that the control loop
  actually moves the toolhead (status broadcasts show encoder X value approaching setX).

Runs against live device and fails if no movement is observed within timeout.

Usage: python3 tests/e2e_movement_test.py --host 192.168.0.12 --port 81 --distance 5
"""
import asyncio
import argparse
import json
import time
import websockets


def parse_status(msg):
    try:
        j = json.loads(msg)
        return j
    except Exception:
        return None


async def recv_status(ws, timeout=5.0):
    start = time.time()
    while time.time() - start < timeout:
        try:
            msg = await asyncio.wait_for(ws.recv(), timeout=timeout - (time.time() - start))
        except Exception:
            return None
        j = parse_status(msg)
        if j and 'x' in j:
            return j
    return None


async def main(host, port, distance_mm):
    uri = f"ws://{host}:{port}"
    print(f"Connecting to {uri}")
    async with websockets.connect(uri) as ws:
        print('Connected')

        # clear to ensure device is not in halted state
        await ws.send('M999')
        print('Sent M999 (clear)')
        # wait until executor_busy is false
        for _ in range(10):
            status = await recv_status(ws, timeout=1.0)
            if status is None:
                continue
            if not status.get('executor_busy'):
                break
        else:
            print('Warning: device did not report executor_idle before test; proceeding anyway')

        # capture initial X
        status = await recv_status(ws, timeout=2.0)
        if not status:
            print('No status, aborting')
            return 2
        x0 = float(status.get('x', 0.0))
        print(f'Initial X = {x0:.3f} mm')

        target = x0 + float(distance_mm)
        cmd = f'G0 X{distance_mm}'
        print('Sending move:', cmd)
        await ws.send(cmd)

        # immediate response may be response JSON
        immediate = None
        try:
            immediate = await asyncio.wait_for(ws.recv(), timeout=1.0)
            print('Immediate recv:', immediate)
        except Exception:
            print('No immediate reply (maybe pending)')

        # Monitor status for movement and final OK
        moved = False
        final_ok = False
        start = time.time()
        while time.time() - start < 15:  # give a generous 15s
            try:
                msg = await asyncio.wait_for(ws.recv(), timeout=2.0)
            except Exception:
                continue
            # check structured response
            j = parse_status(msg)
            if j:
                if 'x' in j:
                    x = float(j['x'])
                    setX = float(j.get('setX', -9999))
                    last_ms = j.get('lastMoveX_ms', None)
                    # movement if x changed towards target by >= 0.5mm
                    if abs(x - x0) >= 0.5:
                        moved = True
                        print(f'status X changed: {x:.3f}mm (set={setX:.3f})')
                    if j.get('executor_busy') is False and abs(setX - x) <= 0.5:
                        # finished and within tolerance
                        print('status indicates finished and in tolerance')
                # also handle response-type messages
                if j.get('type') == 'response' and j.get('status') == 'ok':
                    # final ack for queued/finished responses
                    final_ok = True
                    print('Received final ok response:', j)
                    break
            else:
                # plain text fallback
                if msg.startswith('ok'):
                    final_ok = True
                    print('Received final ok (text)', msg)
                    break

        print('moved=', moved, 'final_ok=', final_ok)
        # test success criteria: movement observed and final ok
        if moved and final_ok:
            print('E2E movement test PASSED')
            return 0
        else:
            print('E2E movement test FAILED (moved:', moved, 'final_ok:', final_ok, ')')
            return 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.0.12')
    parser.add_argument('--port', type=int, default=81)
    parser.add_argument('--distance', type=float, default=5.0)
    args = parser.parse_args()
    rc = asyncio.run(main(args.host, args.port, args.distance))
    raise SystemExit(rc)
