import svgwrite
from svgwrite import cm, mm

def create_wiring_diagram():
    dwg = svgwrite.Drawing('wiring_diagram.svg', size=('400mm', '300mm'))
    
    # Styles
    dwg.defs.add(dwg.style("""
        .text { font-family: Arial; font-size: 10px; }
        .pin-label { font-family: Arial; font-size: 8px; fill: #555; }
        .component-box { fill: #f0f0f0; stroke: #000; stroke-width: 2; }
        .wire { fill: none; stroke-width: 1.5; opacity: 0.7; }
        .wire-pwm { stroke: #ff0000; }
        .wire-dir { stroke: #0000ff; }
        .wire-enc { stroke: #00aa00; }
        .wire-pwr { stroke: #ff0000; stroke-width: 2; }
        .wire-gnd { stroke: #000000; stroke-width: 2; }
        .conflict { fill: red; font-weight: bold; font-size: 12px; }
    """))

    # --- Components ---

    # Lolin32 Lite (Center)
    lolin_x = 150
    lolin_y = 100
    lolin_w = 60
    lolin_h = 120
    dwg.add(dwg.rect(insert=(lolin_x, lolin_y), size=(lolin_w, lolin_h), class_="component-box"))
    dwg.add(dwg.text("Lolin32 Lite", insert=(lolin_x + 5, lolin_y + 15), class_="text"))

    # Pins (Approximate positions for visualization)
    # We will map GPIO numbers to coordinates
    # Left side: 22, 21, 17, 16, 4, 0, 2, 15, 13, 12, 14, 27, 26, 25, 33, 32
    # Right side: 35, 34, 39, 36, 19, 23, 18, 5
    
    pin_map = {}
    left_pins = [22, 21, 17, 16, 4, 0, 2, 15, 13, 12, 14, 27, 26, 25, 33, 32]
    right_pins = [35, 34, 39, 36, 19, 23, 18, 5]
    
    for i, pin in enumerate(left_pins):
        y = lolin_y + 25 + i * 6
        dwg.add(dwg.text(f"G{pin}", insert=(lolin_x + 2, y + 4), class_="pin-label"))
        dwg.add(dwg.circle(center=(lolin_x, y), r=2, fill="black"))
        pin_map[pin] = (lolin_x, y)

    for i, pin in enumerate(right_pins):
        y = lolin_y + 25 + i * 6
        dwg.add(dwg.text(f"G{pin}", insert=(lolin_x + lolin_w - 15, y + 4), class_="pin-label"))
        dwg.add(dwg.circle(center=(lolin_x + lolin_w, y), r=2, fill="black"))
        pin_map[pin] = (lolin_x + lolin_w, y)

    # --- Drivers (L298N) ---
    # 2 Drivers
    
    drivers = [
        {"name": "Driver 1 (X, Y)", "x": 20, "y": 20},
        {"name": "Driver 2 (Z, E)", "x": 280, "y": 20}
    ]

    for d in drivers:
        dwg.add(dwg.rect(insert=(d["x"], d["y"]), size=(80, 100), class_="component-box"))
        dwg.add(dwg.text(d["name"], insert=(d["x"] + 5, d["y"] + 15), class_="text"))
        # L298N Pins
        # Left: ENA, IN1, IN2, IN3, IN4, ENB
        # Right: OUT1, OUT2, OUT3, OUT4
        
        # Draw input terminals
        for i, label in enumerate(["ENA", "IN1", "IN2", "IN3", "IN4", "ENB"]):
            y = d["y"] + 30 + i * 10
            dwg.add(dwg.text(label, insert=(d["x"] + 2, y + 4), class_="pin-label"))
            dwg.add(dwg.circle(center=(d["x"], y), r=2, fill="blue"))
            d[label] = (d["x"], y)

        # Draw output terminals
        for i, label in enumerate(["OUT1", "OUT2", "OUT3", "OUT4"]):
            y = d["y"] + 30 + i * 15
            dwg.add(dwg.text(label, insert=(d["x"] + 75, y + 4), class_="pin-label", text_anchor="end"))
            dwg.add(dwg.circle(center=(d["x"] + 80, y), r=2, fill="orange"))
            d[label] = (d["x"] + 80, y)

    # --- Motors (6-Pin Encoder Motors) ---
    # 4 Motors
    motors = [
        {"name": "X", "driver": 0, "side": "A"},
        {"name": "Y", "driver": 0, "side": "B"},
        {"name": "Z", "driver": 1, "side": "A"},
        {"name": "E", "driver": 1, "side": "B"},
    ]
    
    motor_positions = [
        (20, 150), (120, 150), # X, Y
        (280, 150), (360, 150) # Z, E
    ]

    for i, m in enumerate(motors):
        mx, my = motor_positions[i]
        dwg.add(dwg.rect(insert=(mx, my), size=(60, 50), class_="component-box"))
        dwg.add(dwg.text(f"Motor {m['name']}", insert=(mx + 5, my + 15), class_="text"))
        
        # 6 Pins: M+, M-, 5V, GND, A, B
        pins = ["M+", "M-", "5V", "GND", "A", "B"]
        m["pins"] = {}
        for j, p in enumerate(pins):
            px = mx + 5 + j * 10
            py = my + 45
            dwg.add(dwg.text(p, insert=(px - 2, py - 5), class_="pin-label"))
            dwg.add(dwg.circle(center=(px, py), r=2, fill="green"))
            m["pins"][p] = (px, py)

    # --- MOSFETs (Heaters) ---
    mosfets = [
        {"name": "Hotend FET", "x": 280, "y": 250},
        {"name": "Bed FET", "x": 350, "y": 250}
    ]
    for mos in mosfets:
        dwg.add(dwg.rect(insert=(mos["x"], mos["y"]), size=(40, 40), class_="component-box"))
        dwg.add(dwg.text(mos["name"], insert=(mos["x"] + 2, mos["y"] + 15), class_="text"))
        dwg.add(dwg.text("SIG", insert=(mos["x"] + 2, mos["y"] + 30), class_="pin-label"))
        dwg.add(dwg.circle(center=(mos["x"], mos["y"] + 30), r=2, fill="red"))
        mos["sig"] = (mos["x"], mos["y"] + 30)

    # --- Connections (From config.h) ---
    # UPDATED PINOUT (4-Axis, Full Quadrature Encoders)
    
    # X: IN1(22), IN2(21), A(34), B(35)
    # Y: IN1(19), IN2(23), A(36), B(39)
    # Z: IN1(18), IN2(5),  A(25), B(26)
    # E: IN1(17), IN2(16), A(27), B(14)
    # Heaters: Hotend(4), Bed(13)
    
    connections = [
        # X (Driver 1)
        (22, drivers[0], "IN1", "in1"), (21, drivers[0], "IN2", "in2"), 
        (34, motors[0]["pins"], "A", "enc"), (35, motors[0]["pins"], "B", "enc"),
        
        # Y (Driver 1)
        (19, drivers[0], "IN3", "in1"), (23, drivers[0], "IN4", "in2"),
        (36, motors[1]["pins"], "A", "enc"), (39, motors[1]["pins"], "B", "enc"),
        
        # Z (Driver 2)
        (18, drivers[1], "IN1", "in1"), (5, drivers[1], "IN2", "in2"),
        (25, motors[2]["pins"], "A", "enc"), (26, motors[2]["pins"], "B", "enc"),
        
        # E (Driver 2)
        (17, drivers[1], "IN3", "in1"), (16, drivers[1], "IN4", "in2"),
        (27, motors[3]["pins"], "A", "enc"), (14, motors[3]["pins"], "B", "enc"),
        
        # Heaters
        (4, mosfets[0], "sig", "pwm"),
        (13, mosfets[1], "sig", "pwm")
    ]

    # --- Routing Helper ---
    def draw_angled_wire(dwg, start, end, color, lane_offset=0, side="left"):
        """
        Draws an orthogonal wire path.
        side: 'left' or 'right' relative to the source (ESP32).
        lane_offset: spacing for the vertical channel.
        """
        points = [start]
        
        # Channel X coordinate
        if side == "left":
            channel_x = start[0] - 20 - (lane_offset * 3)
        else:
            channel_x = start[0] + 20 + (lane_offset * 3)
            
        # 1. Move horizontally to channel
        points.append((channel_x, start[1]))
        
        # 2. Move vertically to target Y
        points.append((channel_x, end[1]))
        
        # 3. Move horizontally to target
        points.append(end)
        
        # Draw polyline
        dwg.add(dwg.polyline(points, stroke=color, stroke_width=1.5, fill="none", opacity=0.8))

    # Draw Wires
    used_pins = {}
    left_lane_idx = 0
    right_lane_idx = 0
    
    # 1. Logic Connections (ESP32 -> Drivers/Encoders)
    for pin, comp, port, type_ in connections:
        if pin not in pin_map:
            continue
            
        start = pin_map[pin]
        if isinstance(comp, dict) and port in comp:
            end = comp[port]
        else:
            continue
            
        # Color
        color = "black"
        if type_ == "pwm": color = "red"
        elif type_ == "dir": color = "blue"
        elif type_ == "enc": color = "green"
        elif type_ == "in1": color = "purple"
        elif type_ == "in2": color = "orange"
        
        # Determine side and lane
        # Lolin is at x=150, width=60. Center ~180.
        # Left pins are at x=150. Right pins are at x=210.
        if start[0] <= 150:
            side = "left"
            lane = left_lane_idx
            left_lane_idx += 1
        else:
            side = "right"
            lane = right_lane_idx
            right_lane_idx += 1
            
        draw_angled_wire(dwg, start, end, color, lane, side)
        
        # Track usage for conflicts
        if pin not in used_pins:
            used_pins[pin] = []
        used_pins[pin].append(f"{port}")

    # 2. Power Connections (Drivers -> Motors)
    # Simple vertical routing with small offset
    
    power_connections = [
        (drivers[0], "OUT1", motors[0]["pins"], "M+"), (drivers[0], "OUT2", motors[0]["pins"], "M-"),
        (drivers[0], "OUT3", motors[1]["pins"], "M+"), (drivers[0], "OUT4", motors[1]["pins"], "M-"),
        (drivers[1], "OUT1", motors[2]["pins"], "M+"), (drivers[1], "OUT2", motors[2]["pins"], "M-"),
        (drivers[1], "OUT3", motors[3]["pins"], "M+"), (drivers[1], "OUT4", motors[3]["pins"], "M-"),
    ]

    pwr_idx = 0
    for driver, d_port, motor_pins, m_port in power_connections:
        start = driver[d_port]
        end = motor_pins[m_port]
        
        # Route: Out -> Right -> Down -> Left -> In
        # Driver output is at x + 80
        # Motor input is at various x
        
        points = [start]
        
        # Move right a bit to clear terminals
        mid_x = start[0] + 10 + (pwr_idx % 2) * 5
        points.append((mid_x, start[1]))
        
        # Move down to motor height
        points.append((mid_x, end[1] - 15))
        
        # Move to motor x
        points.append((end[0], end[1] - 15))
        
        # Move down to pin
        points.append(end)
        
        dwg.add(dwg.polyline(points, stroke="orange", stroke_width=2, fill="none", opacity=0.8))
        pwr_idx += 1

    # Highlight Conflicts
    conflict_y = 250
    dwg.add(dwg.text("CONFLICTS DETECTED IN CONFIG.H:", insert=(150, conflict_y), class_="conflict"))
    
    count = 0
    for pin, usages in used_pins.items():
        if len(usages) > 1:
            txt = f"GPIO {pin}: Used by {', '.join(usages)}"
            dwg.add(dwg.text(txt, insert=(150, conflict_y + 15 + count * 12), fill="red", font_size="10px"))
            count += 1
            
            # Highlight pin on board
            if pin in pin_map:
                dwg.add(dwg.circle(center=pin_map[pin], r=4, fill="none", stroke="red", stroke_width=2))

    dwg.save()

if __name__ == "__main__":
    create_wiring_diagram()
