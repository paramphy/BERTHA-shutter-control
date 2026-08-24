# Vacuum Shutter Controller

A simple and robust Arduino-based controller for operating two stepper-motor-driven shutters in a high-vacuum sputtering system.

The controller operates two shutters simultaneously between two predefined positions:

* **CLOSED**
* **OPEN**

A single push button toggles between the two states. Two LEDs provide a visual indication of the current shutter state.

The design intentionally keeps the firmware simple and deterministic. There is no display, network connection, command queue, or unnecessary control logic.

---

## Features

* Control of two stepper motors simultaneously
* STEP/DIR motor-driver interface
* Single-button OPEN/CLOSE operation
* Smooth acceleration and deceleration using `AccelStepper`
* Software-defined OPEN and CLOSED positions
* Button debouncing
* Button input ignored while shutters are moving
* Two-state LED indication
* Serial status messages for basic debugging
* Conservative motion parameters for reliable operation
* No endstop switches required

---

## System Overview

The system consists of:

```text
                    ┌───────────────────┐
                    │      Button       │
                    │      Pin 12       │
                    └─────────┬─────────┘
                              │
                              ▼
                    ┌───────────────────┐
                    │      Arduino      │
                    │                   │
                    │ Shutter Controller│
                    └───────┬─────┬─────┘
                            │     │
                 STEP/DIR   │     │   STEP/DIR
                            │     │
                            ▼     ▼
                       ┌──────┐ ┌──────┐
                       │Driver│ │Driver│
                       │  1   │ │  2   │
                       └──┬───┘ └──┬───┘
                          │        │
                          ▼        ▼
                       Motor 1  Motor 2
                          │        │
                          └───┬────┘
                              │
                         Shutters
                             
              ┌────────────────────────────┐
              │                            │
              │  CLOSED LED      OPEN LED  │
              │    Pin 8          Pin 9    │
              │                            │
              └────────────────────────────┘
```

Both motors are commanded to move to the same target position. The controller considers the operation complete only after **both motors have reached their target positions**.

---

# Hardware

## Main Components

* Arduino-compatible microcontroller
* 2 × stepper motors
* 2 × STEP/DIR stepper motor drivers
* 1 × momentary push button
* 2 × red LEDs
* 2 × LED current-limiting resistors
* Vacuum shutter mechanism
* Appropriate motor power supply

---

## Stepper Motors

The firmware assumes that both motors are controlled through external STEP/DIR drivers.

The Arduino does **not** directly drive the stepper motor coils.

The motor drivers should provide:

* STEP input
* DIR input
* Appropriate motor current regulation
* Configurable microstepping

---

## Microstepping

The current position calculation assumes:

* 800 full steps per revolution
* 1/32 microstepping
* 90° shutter rotation

Therefore:

```text
800 × 32 = 25,600 microsteps/revolution

25,600 / 4 = 6,400 microsteps for 90°
```

The firmware therefore uses:

```cpp
const long CLOSED_POSITION = 0;
const long OPEN_POSITION   = 6400;
```

If the motor-driver microstepping configuration is changed, `OPEN_POSITION` must also be recalculated.

---

# Pin Configuration

| Arduino Pin | Function       |
| ----------: | -------------- |
|           2 | Stepper 2 DIR  |
|           3 | Stepper 1 DIR  |
|           5 | Stepper 2 STEP |
|           6 | Stepper 1 STEP |
|           8 | CLOSED LED     |
|           9 | OPEN LED       |
|          12 | Push button    |

---

## Motor Connections

### Shutter Motor 1

```text
Arduino Pin 6 → STEP
Arduino Pin 3 → DIR
```

### Shutter Motor 2

```text
Arduino Pin 5 → STEP
Arduino Pin 2 → DIR
```

The actual motor coils are connected to the respective stepper drivers, not directly to the Arduino.

---

# LED Connections

Two red LEDs indicate the state of the shutter system.

### CLOSED LED

```text
Arduino Pin 8
     │
     └── 330 Ω resistor ── LED ── GND
```

### OPEN LED

```text
Arduino Pin 9
     │
     └── 330 Ω resistor ── LED ── GND
```

A resistor should be used for each LED.

A value between approximately **220 Ω and 330 Ω** is suitable for typical indicator LEDs. 330 Ω is recommended for a conservative implementation.

---

# Button Connection

The button uses the Arduino's internal pull-up resistor.

```text
Arduino Pin 12 ───── Button ───── GND
```

The firmware configures the pin as:

```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

Therefore:

```text
Button released → HIGH
Button pressed  → LOW
```

---

# Software

## Requirements

The firmware requires:

* Arduino IDE or PlatformIO
* Arduino-compatible board
* C++ compiler supported by the Arduino environment
* `AccelStepper` library

### AccelStepper

The firmware uses the `AccelStepper` library for:

* STEP/DIR control
* Speed control
* Acceleration/deceleration
* Position tracking
* Non-blocking motor operation

---

# Installation

## Arduino IDE

1. Install the Arduino IDE.
2. Install the `AccelStepper` library through the Library Manager.
3. Connect the Arduino.
4. Open the project.
5. Select the appropriate Arduino board.
6. Select the correct serial port.
7. Compile the firmware.
8. Upload the firmware.

---

## PlatformIO

The project can also be built using PlatformIO.

The required library is:

```text
AccelStepper
```

A typical PlatformIO configuration should include the appropriate board and:

```ini
lib_deps =
    waspinator/AccelStepper
```

---

# Operating Principle

The controller has three internal states:

```text
SHUTTER_CLOSED
SHUTTER_OPEN
SHUTTER_MOVING
```

## CLOSED

When the shutters are closed:

```text
CLOSED LED → ON
OPEN LED   → OFF
```

Pressing the button commands both motors to move to `OPEN_POSITION`.

---

## MOVING

While the shutters are moving:

```text
CLOSED LED → OFF
OPEN LED   → OFF
```

The controller continuously calls:

```cpp
shutter1.run();
shutter2.run();
```

The button is ignored while the shutters are moving.

This prevents accidental repeated commands during an OPEN/CLOSE operation.

---

## OPEN

When both motors reach the open position:

```text
CLOSED LED → OFF
OPEN LED   → ON
```

Pressing the button again commands both motors to return to the closed position.

---

# Motion Parameters

The current firmware uses conservative motion settings:

```cpp
const float MAX_SPEED    = 2000.0;
const float ACCELERATION = 100.0;
```

These values were intentionally chosen to prioritize smooth and reliable operation over maximum speed.

For a vacuum shutter, rapid movement is generally unnecessary. Excessive acceleration can introduce:

* Mechanical vibration
* Stepper resonance
* Missed steps
* Mechanical shock
* Unnecessary stress on the shutter mechanism

If the system operates reliably at these values, they should preferably be left unchanged.

---

# Position Parameters

The current positions are:

```cpp
const long CLOSED_POSITION = 0;
const long OPEN_POSITION   = 6400;
```

The software assumes that:

```text
0 steps      → CLOSED
6400 steps   → OPEN
```

The Arduino establishes the initial position as CLOSED during startup:

```cpp
shutter1.setCurrentPosition(CLOSED_POSITION);
shutter2.setCurrentPosition(CLOSED_POSITION);
```

### Important

Because this implementation does **not** use physical endstop switches, the controller assumes that the mechanical system is physically in the CLOSED position when the Arduino is powered or reset.

The software position is therefore based on the assumption that the mechanical position has not been changed while the controller was unpowered.

---

# LED State Table

| Shutter state | CLOSED LED | OPEN LED |
| ------------- | ---------: | -------: |
| Closed        |         ON |      OFF |
| Moving        |        OFF |      OFF |
| Open          |        OFF |       ON |

The LEDs indicate the **completed software state**, not the commanded state.

For example, when the OPEN command is issued, the OPEN LED does not immediately turn on. Both LEDs remain off until both motors reach the open position.

This makes the indication unambiguous.

---

# Button Behaviour

The button operates as a toggle:

```text
CLOSED
   │
   │ button
   ▼
OPEN
   │
   │ button
   ▼
CLOSED
```

Additional button presses during movement are ignored.

The button also uses a short software debounce period:

```cpp
const unsigned long DEBOUNCE_TIME = 50;
```

This prevents mechanical button bounce from being interpreted as multiple commands.

---

# Serial Output

The controller uses the serial port at:

```cpp
Serial.begin(9600);
```

Example output:

```text
==============================
Shutter controller ready
State: CLOSED
==============================
```

During operation:

```text
Opening shutters...
State: OPEN
```

and:

```text
Closing shutters...
State: CLOSED
```

The serial output is intended primarily for commissioning and troubleshooting.

---

# Safety and Reliability Considerations

This controller is intentionally designed as a **simple motion controller**, rather than a complete safety system.

Before connecting the system to the actual vacuum shutter mechanism:

1. Test the motors without mechanical load.
2. Verify the STEP/DIR wiring.
3. Verify motor-driver current settings.
4. Verify the microstepping configuration.
5. Verify the OPEN position.
6. Verify the CLOSED position.
7. Test repeated OPEN/CLOSE cycles.
8. Check for mechanical interference.
9. Confirm that the motor does not lose steps.
10. Confirm that the shutter cannot mechanically over-travel.

The motor current and driver configuration should be appropriate for the selected stepper motors.

---

# Why No Endstop?

This implementation intentionally does not use endstop switches.

The shutter mechanism uses predefined software positions:

```text
CLOSED_POSITION = 0
OPEN_POSITION   = 6400
```

This keeps the electronics and firmware simple.

However, because there is no physical position feedback, the system relies on the motor maintaining its commanded position.

If the mechanical design later requires absolute position verification, physical endstops or another position sensor can be added.

---

# Design Philosophy

The controller is deliberately kept simple.

The intended control sequence is:

```text
Button press
     ↓
Determine current state
     ↓
Select target position
     ↓
Move both motors
     ↓
Wait until both reach target
     ↓
Update state
     ↓
Update LED
     ↓
Wait for next button press
```

Features such as:

* Wi-Fi
* displays
* complex command queues
* automatic calibration
* PID control
* remote control

are intentionally excluded from this firmware.

The goal is to provide a small, predictable controller that can operate the vacuum shutters reliably without unnecessary software complexity.

---

# Project Structure

A simple repository structure is recommended:

```text
vacuum-shutter-controller/
│
├── src/
│   └── main.cpp
│
├── README.md
│
├── platformio.ini
│
└── LICENSE
```

For an Arduino IDE project, `main.cpp` can instead be maintained as an `.ino` file.

---

# Current Firmware Configuration

The primary configuration values are:

```cpp
const long CLOSED_POSITION = 0;
const long OPEN_POSITION   = 6400;

const float MAX_SPEED      = 2000.0;
const float ACCELERATION   = 100.0;

const unsigned long DEBOUNCE_TIME = 50;
```

Hardware pins:

```cpp
const uint8_t BUTTON_PIN     = 12;
const uint8_t CLOSED_LED_PIN = 8;
const uint8_t OPEN_LED_PIN   = 9;
```

---

# Troubleshooting

## Motor vibrates or resonates

Try reducing:

```cpp
MAX_SPEED
```

and/or:

```cpp
ACCELERATION
```

Acceleration should generally be reduced before increasing motor speed.

Also check:

* Motor-driver current
* Microstepping configuration
* Motor supply voltage
* Mechanical coupling
* Loose mounting
* Mechanical resonance

---

## Motor moves in the wrong direction

Reverse the motor direction in the firmware or swap the motor's direction configuration according to the driver.

Do not change the STEP and DIR pins unless necessary.

---

## Shutters do not open the correct amount

Check:

```cpp
OPEN_POSITION
```

The value depends on:

* Motor step angle
* Driver microstepping
* Gear ratio
* Mechanical transmission
* Required shutter rotation

---

## LED is not working

Check:

1. LED polarity.
2. Resistor connection.
3. Ground connection.
4. Correct Arduino pin.
5. LED orientation.

The longer LED leg is normally the anode (+).

---

## Button triggers multiple times

The firmware already includes a 50 ms debounce period.

If necessary, increase:

```cpp
DEBOUNCE_TIME
```

but normally 50 ms should be sufficient.

---

# Future Improvements

Possible future additions, if required by the experimental system, include:

* Physical endstop detection
* Hardware emergency stop
* Motor fault monitoring
* External OPEN/CLOSE control
* Computer/USB control
* Position verification
* Vacuum-system interlock
* Logging of shutter operations

These are intentionally not part of the current minimal implementation.

---

# License

Add the appropriate license for the project.

For example:

```text
MIT License
```

if the firmware is intended to be openly reused and modified.

---

## Status

**Development status:** Functional prototype / laboratory implementation

The controller has been designed for operation of two stepper-motor-driven shutters in a sputtering/vacuum-system environment.

The firmware prioritizes:

* Simplicity
* Repeatability
* Smooth motion
* Minimal dependencies
* Easy maintenance
* Clear state indication
