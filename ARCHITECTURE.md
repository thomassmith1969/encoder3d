# Encoder3D Architecture and Design

## System Overview

Encoder3D uses a **non-blocking, event-driven architecture** to ensure real-time control is never interrupted by I/O or communication operations.

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32 Dual Core                          │
├──────────────────────────┬──────────────────────────────────────┤
│  CORE 0 (Protocol CPU)   │  CORE 1 (Application CPU)            │
│                          │                                      │
│  ┌────────────────────┐  │  ┌────────────────────────────────┐ │
│  │ Heater Control     │  │  │ Motor Control Loop             │ │
│  │ Loop (10 Hz)       │  │  │ (1000 Hz - HIGH PRIORITY)      │ │
│  │ - PID updates      │  │  │ - Read encoders                │ │
│  │ - Safety checks    │  │  │ - Update PIDs                  │ │
│  │ - Thermal runaway  │  │  │ - Apply motor outputs          │ │
│  └────────────────────┘  │  │ - Motion planning              │ │
│                          │  └────────────────────────────────┘ │
│                          │                                      │
│  Main Loop (async)       │  NO BLOCKING OPERATIONS             │
│  ┌────────────────────┐  │  ALLOWED IN THESE TASKS            │
│  │ WiFi/Network       │  │                                      │
│  │ - Web Server       │  │                                      │
│  │ - WebSocket        │  │                                      │
│  │ - Telnet           │  │                                      │
│  │ Serial I/O         │  │                                      │
│  │ G-code Parser      │  │                                      │
│  └────────────────────┘  │                                      │
└──────────────────────────┴──────────────────────────────────────┘
```

## Core Design Principles

### 1. **No Blocking Operations**
❌ **Never use:**
- `delay()` in main loop or tasks
- `while()` loops waiting for conditions
- Blocking I/O (Serial.readString, client.read() in loops)
- File operations in main loop

✅ **Instead use:**
- `yield()` to give up CPU slice
- State machines for multi-step operations  
- Callbacks or flags
- FreeRTOS queues/semaphores for inter-task communication

### 2. **Separation of Concerns**

#### Real-Time Tasks (MUST NOT BLOCK)
- Motor control loop (1kHz)
- Heater control loop (10Hz)

#### Non-Real-Time (Can tolerate latency)
- Network I/O
- Serial I/O  
- G-code parsing
- File operations

### 3. **Event-Driven Communication**

```
Client Request → Queue → Process when ready → Response
     (async)              (non-blocking)        (async)
```

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Main Loop (Core 0)                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │
│  │ Web Server   │  │ Telnet       │  │ Serial       │             │
│  │ .update()    │  │ .update()    │  │ (polled)     │             │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘             │
│         │                 │                 │                       │
│         └─────────────────┴─────────────────┘                       │
│                           ▼                                         │
│                  ┌─────────────────┐                                │
│                  │  G-code Parser  │                                │
│                  │  (stateless)    │                                │
│                  └────────┬────────┘                                │
│                           │                                         │
│         ┌─────────────────┼─────────────────┐                      │
│         ▼                 ▼                 ▼                        │
│  ┌────────────┐   ┌────────────┐   ┌────────────┐                 │
│  │  Motor     │   │  Heater    │   │  System    │                 │
│  │  Commands  │   │  Commands  │   │  Commands  │                 │
│  └─────┬──────┘   └─────┬──────┘   └────────────┘                 │
└────────┼────────────────┼─────────────────────────────────────────┘
         │                │
         │ Set target     │ Set temp
         │ (non-blocking) │ (non-blocking)
         │                │
┌────────▼────────────────▼─────────────────────────────────────────┐
│                    Real-Time Tasks                                 │
│  ┌──────────────────────────┐  ┌──────────────────────────┐       │
│  │ Motor Control (Core 1)   │  │ Heater Control (Core 0)  │       │
│  │ - Read encoder           │  │ - Read thermistor        │       │
│  │ - Compute PID            │  │ - Compute PID            │       │
│  │ - Apply PWM              │  │ - Apply PWM              │       │
│  │ - Update position        │  │ - Safety checks          │       │
│  └──────────────────────────┘  └──────────────────────────┘       │
└────────────────────────────────────────────────────────────────────┘
```

## Data Flow: Non-Blocking Example

### Example: G-code Move Command

```
1. Web UI sends: "G1 X10 Y20 F1000"
   └─> WebSocket (async)
   
2. WebServer.onWebSocketEvent()
   └─> Parse message (no blocking)
   └─> gcodeParser.processLine()  (stateless, immediate)
   
3. GCodeParser.handleG0G1()
   └─> motorController.linearMove(x, y, ...)  (sets targets only)
   └─> Returns immediately (does NOT wait for motion)
   
4. Motor Control Task (running independently @ 1kHz)
   └─> Reads current position from encoders
   └─> Computes path toward target
   └─> Updates motor PWM
   └─> Repeats until target reached
   
5. Status updates (periodic, non-blocking)
   └─> Main loop checks isMoving() every 500ms
   └─> Broadcasts status via WebSocket
```

**Total blocking time in main loop: ~0ms**

## Anti-Patterns Found and Fixed

### ❌ Original Anti-Pattern
```cpp
void handleM109(float temp) {
    setTemperature(temp);
    while (!atTarget()) {
        delay(100);  // BLOCKS EVERYTHING!
    }
}
```

**Problems:**
- Blocks main loop for minutes
- Network clients timeout
- Motor control jitters
- WebSocket disconnects
- Serial buffer overflows

### ✅ Correct Pattern
```cpp
void handleM109(float temp) {
    setTemperature(temp);
    sendResponse("Heating to " + String(temp));
    // Client polls M105 for status
    // System continues processing other requests
}
```

**Benefits:**
- Main loop stays responsive
- Multiple clients can connect
- Real-time tasks unaffected
- Client handles waiting logic

## State Machine Pattern for Complex Operations

For operations that require multiple steps, use state machines:

```cpp
enum HomingState {
    HOMING_IDLE,
    HOMING_MOVE_OFF_SWITCH,
    HOMING_SEARCH_SWITCH,
    HOMING_SLOW_APPROACH,
    HOMING_BACKOFF,
    HOMING_COMPLETE
};

class HomingStateMachine {
    HomingState state;
    unsigned long stateStartTime;
    
public:
    void start() {
        state = HOMING_MOVE_OFF_SWITCH;
        stateStartTime = millis();
    }
    
    void update() {  // Called from main loop
        switch(state) {
            case HOMING_IDLE:
                break;
                
            case HOMING_MOVE_OFF_SWITCH:
                if (!endstopTriggered()) {
                    state = HOMING_SEARCH_SWITCH;
                }
                break;
                
            case HOMING_SEARCH_SWITCH:
                if (endstopTriggered()) {
                    state = HOMING_SLOW_APPROACH;
                }
                if (millis() - stateStartTime > TIMEOUT) {
                    state = HOMING_IDLE;
                    reportError("Homing timeout");
                }
                break;
                
            // ... more states
        }
    }
};
```

## Buffer Management

All I/O uses bounded buffers with overflow handling:

```cpp
// Good: Bounded with overflow handling
if (Serial.availableForWrite() > messageLength) {
    Serial.println(message);
} else {
    // Drop message or queue for later
    droppedMessages++;
}

// Bad: Unbounded blocking
Serial.println(message);  // Blocks if buffer full!
```

## Performance Targets

| Component | Target | Critical? |
|-----------|--------|-----------|
| Motor loop | 1000 Hz (1ms) | YES - Hard real-time |
| Heater loop | 10 Hz (100ms) | YES - Safety critical |
| Network I/O | Best effort | NO - Can tolerate latency |
| Serial I/O | Best effort | NO - Can drop messages |
| Status broadcast | 2 Hz (500ms) | NO - Can skip updates |

## Critical Section Protection

When accessing shared data between tasks:

```cpp
// Use FreeRTOS primitives
portENTER_CRITICAL(&spinlock);
sharedVariable = newValue;
portEXIT_CRITICAL(&spinlock);

// Or use atomic operations
std::atomic<float> currentPosition;
```

## Testing for Blocking

To detect blocking operations:

1. **Watchdog Timer**: Should never trigger
2. **Loop timing**: Main loop should complete in <10ms
3. **Task jitter**: Control loops should have <1% jitter
4. **Network stress test**: Flood with requests while running
5. **Serial flood test**: Send continuous data

```cpp
// Add to main loop for debugging
unsigned long loopStart = micros();
// ... loop code ...
unsigned long loopTime = micros() - loopStart;
if (loopTime > 10000) {  // 10ms
    Serial.println("WARNING: Loop blocked for " + String(loopTime) + "us");
}
```

## Summary

The Encoder3D architecture ensures:

✅ **Real-time control never interrupted** by I/O  
✅ **Network operations are fully async**  
✅ **No blocking waits** in any code path  
✅ **Bounded buffers** prevent memory issues  
✅ **State machines** handle multi-step operations  
✅ **Proper task separation** via dual-core architecture  

This design allows the system to:
- Control motors with microsecond precision
- Handle multiple network clients simultaneously
- Process G-code while moving
- Maintain safety monitoring continuously
- Never miss encoder pulses or overheat

---

**Design Rule**: If it can block, it doesn't belong in the main loop or real-time tasks.
