#include <Arduino.h>
#include <AccelStepper.h>

// ============================================================
// Shutter Controller
// ============================================================
//
// Two stepper motors operate two shutters simultaneously.
//
// Button:
//   Pin 12
//
// Stepper 1:
//   STEP = 6
//   DIR  = 3
//
// Stepper 2:
//   STEP = 5
//   DIR  = 2
//
// LEDs:
//   CLOSED LED = Pin 8
//   OPEN LED   = Pin 9
//
// ============================================================


// ------------------------------------------------------------
// Motor definitions
// ------------------------------------------------------------

AccelStepper shutter1(AccelStepper::DRIVER, 6, 3);
AccelStepper shutter2(AccelStepper::DRIVER, 5, 2);


// ------------------------------------------------------------
// Hardware pins
// ------------------------------------------------------------

const uint8_t BUTTON_PIN      = 12;
const uint8_t CLOSED_LED_PIN  = 8;
const uint8_t OPEN_LED_PIN    = 9;


// ------------------------------------------------------------
// Shutter positions
// ------------------------------------------------------------
//
// 800 full steps/revolution
// 1/32 microstepping
// 90 degree rotation
//
// 800 * 32 / 4 = 6400 microsteps
// ------------------------------------------------------------

const long CLOSED_POSITION = 0;
const long OPEN_POSITION   = 6400;


// ------------------------------------------------------------
// Motor motion parameters
// ------------------------------------------------------------
//
// Conservative starting values.
//
// The shutter does not need to move very quickly.
// Smooth and reliable operation is more important.
//
// ------------------------------------------------------------

const float MAX_SPEED    = 2000.0;
const float ACCELERATION = 100.0;


// ------------------------------------------------------------
// Shutter state
// ------------------------------------------------------------

enum ShutterState
{
    SHUTTER_CLOSED,
    SHUTTER_OPEN,
    SHUTTER_MOVING
};

ShutterState shutterState = SHUTTER_CLOSED;


// ------------------------------------------------------------
// Button debounce
// ------------------------------------------------------------

bool lastButtonState = HIGH;
unsigned long lastButtonChangeTime = 0;

const unsigned long DEBOUNCE_TIME = 50;


// ============================================================
// Update LEDs
// ============================================================

void updateLEDs()
{
    switch (shutterState)
    {
        case SHUTTER_CLOSED:

            digitalWrite(CLOSED_LED_PIN, HIGH);
            digitalWrite(OPEN_LED_PIN, LOW);

            break;


        case SHUTTER_OPEN:

            digitalWrite(CLOSED_LED_PIN, LOW);
            digitalWrite(OPEN_LED_PIN, HIGH);

            break;


        case SHUTTER_MOVING:

            digitalWrite(CLOSED_LED_PIN, LOW);
            digitalWrite(OPEN_LED_PIN, LOW);

            break;
    }
}


// ============================================================
// Open shutters
// ============================================================

void openShutters()
{
    Serial.println("Opening shutters...");

    // Turn both LEDs off while moving
    shutterState = SHUTTER_MOVING;
    updateLEDs();

    shutter1.moveTo(OPEN_POSITION);
    shutter2.moveTo(OPEN_POSITION);
}


// ============================================================
// Close shutters
// ============================================================

void closeShutters()
{
    Serial.println("Closing shutters...");

    // Turn both LEDs off while moving
    shutterState = SHUTTER_MOVING;
    updateLEDs();

    shutter1.moveTo(CLOSED_POSITION);
    shutter2.moveTo(CLOSED_POSITION);
}


// ============================================================
// Update motors
// ============================================================

void updateMotors()
{
    // Keep both motors running
    shutter1.run();
    shutter2.run();


    // --------------------------------------------------------
    // Check whether both motors have reached their targets
    // --------------------------------------------------------

    if (shutterState == SHUTTER_MOVING)
    {
        if (shutter1.distanceToGo() == 0 &&
            shutter2.distanceToGo() == 0)
        {
            // Both shutters are fully OPEN
            if (shutter1.currentPosition() == OPEN_POSITION &&
                shutter2.currentPosition() == OPEN_POSITION)
            {
                shutterState = SHUTTER_OPEN;

                updateLEDs();

                Serial.println("State: OPEN");
            }

            // Both shutters are fully CLOSED
            else if (shutter1.currentPosition() == CLOSED_POSITION &&
                     shutter2.currentPosition() == CLOSED_POSITION)
            {
                shutterState = SHUTTER_CLOSED;

                updateLEDs();

                Serial.println("State: CLOSED");
            }
        }
    }
}


// ============================================================
// Update button
// ============================================================

void updateButton()
{
    bool currentButtonState = digitalRead(BUTTON_PIN);


    // --------------------------------------------------------
    // Detect button state change
    // --------------------------------------------------------

    if (currentButtonState != lastButtonState)
    {
        lastButtonChangeTime = millis();
        lastButtonState = currentButtonState;
    }


    // --------------------------------------------------------
    // Wait for debounce
    // --------------------------------------------------------

    if ((millis() - lastButtonChangeTime) < DEBOUNCE_TIME)
    {
        return;
    }


    // --------------------------------------------------------
    // Button is active LOW
    // --------------------------------------------------------

    if (currentButtonState == LOW)
    {
        // Ignore button presses while shutters are moving
        if (shutterState == SHUTTER_MOVING)
        {
            return;
        }


        // ----------------------------------------------------
        // CLOSED → OPEN
        // ----------------------------------------------------

        if (shutterState == SHUTTER_CLOSED)
        {
            openShutters();
        }


        // ----------------------------------------------------
        // OPEN → CLOSED
        // ----------------------------------------------------

        else if (shutterState == SHUTTER_OPEN)
        {
            closeShutters();
        }


        // ----------------------------------------------------
        // Wait until button is released
        // ----------------------------------------------------

        while (digitalRead(BUTTON_PIN) == LOW)
        {
            updateMotors();
        }

        lastButtonState = HIGH;
    }
}


// ============================================================
// Setup
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Button
    // --------------------------------------------------------

    pinMode(BUTTON_PIN, INPUT_PULLUP);


    // --------------------------------------------------------
    // LEDs
    // --------------------------------------------------------

    pinMode(CLOSED_LED_PIN, OUTPUT);
    pinMode(OPEN_LED_PIN, OUTPUT);


    // --------------------------------------------------------
    // Serial
    // --------------------------------------------------------

    Serial.begin(9600);


    // --------------------------------------------------------
    // Configure motors
    // --------------------------------------------------------

    shutter1.setMaxSpeed(MAX_SPEED);
    shutter1.setAcceleration(ACCELERATION);

    shutter2.setMaxSpeed(MAX_SPEED);
    shutter2.setAcceleration(ACCELERATION);


    // --------------------------------------------------------
    // Initial software position
    // --------------------------------------------------------

    shutter1.setCurrentPosition(CLOSED_POSITION);
    shutter2.setCurrentPosition(CLOSED_POSITION);

    shutterState = SHUTTER_CLOSED;


    // --------------------------------------------------------
    // Initial LED state
    // --------------------------------------------------------

    updateLEDs();


    // --------------------------------------------------------
    // Startup message
    // --------------------------------------------------------

    Serial.println();
    Serial.println("==============================");
    Serial.println("Shutter controller ready");
    Serial.println("State: CLOSED");
    Serial.println("==============================");
}


// ============================================================
// Main loop
// ============================================================

void loop()
{
    updateMotors();
    updateButton();
}