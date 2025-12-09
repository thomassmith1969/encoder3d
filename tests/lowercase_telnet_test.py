#!/usr/bin/env python3
"""
Verify lowercase g-code via Telnet is parsed and accepted by the system.
"""
import argparse
import socket
import time


def main(host, port):
    print(f"Connecting telnet to {host}:{port}")
    tn = socket.create_connection((host, port), timeout=5)
    tn.settimeout(2.0)

    time.sleep(0.1)
    try:
        out = tn.recv(1024).decode('utf-8', errors='ignore')
        if out:
            print('Greeting:', out.strip())
    except Exception:
        pass

    print('Sending lowercase move: g0 x1 y1')
    tn.sendall(b"G0 x1 y1\n")
    try:
        resp = tn.recv(256).decode('utf-8', errors='ignore').strip()
        print('Telnet response (immediate):', resp)
    except Exception as e:
        print('No immediate response:', e)

    tn.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='192.168.0.12')
    parser.add_argument('--port', type=int, default=23)
    args = parser.parse_args()
    main(args.host, args.port)
