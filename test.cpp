#include <MCP_CAN.h>
#include <SPI.h>

MCP_CAN CAN(9); // CAN interface connected to pin 9

const uint32_t cruiseControlButtonID = 0x200; // CAN ID for cruise control button messages
const uint32_t plusButtonID = 0x201; // CAN ID for "+" button messages
const uint32_t minusButtonID = 0x202; // CAN ID for "-" button messages
const uint32_t forcedRPMPositionID = 0x300; // CAN ID for forced RPM position messages

unsigned long buttonPressStartTime = 0; // Time when button was pressed
bool buttonPressed = false; // Track if the button is currently pressed
const unsigned long holdDuration = 3000; // Duration to hold button in milliseconds
bool forceTachometer = false; // Flag to indicate if we should force the RPM
int forcedRPM = 1000; // Starting RPM in forced state
int savedRPM = 1000; // Variable to save the last RPM before forcing
int previousForcedRPM = 1000; // Track last forced RPM position to prevent redundant CAN sends

void setup() {
    Serial.begin(115200);
    CAN.begin(500E3); // Initialize CAN at 500 kbps
}

void loop() {
    // Check for incoming CAN messages
    if (CAN_MSGAVAIL == CAN.checkReceive()) {
        unsigned char len = 0;
        unsigned char buf[8];
        uint32_t canId = 0;

        CAN.readMsgBuf(&canId, &len, buf); // Read CAN message

        // Check if the cruise control button is pressed
        if (canId == cruiseControlButtonID && buf[0] == 1) {
            // Button is pressed
            if (!buttonPressed) {
                buttonPressed = true; // Button was just pressed
                buttonPressStartTime = millis(); // Start timing
            }

            // Check if the button has been held for the required duration
            if (millis() - buttonPressStartTime >= holdDuration) {
                forceTachometer = !forceTachometer; // Toggle forced tachometer state
                if (forceTachometer) {
                    savedRPM = readActualRPM(); // Save the current actual RPM
                    forcedRPM = 1000; // Reset forced RPM to 1000
                }
                previousForcedRPM = -1; // Reset to ensure next send is fresh
            }
        } else if (canId == cruiseControlButtonID && buf[0] == 0) {
            // Button released
            buttonPressed = false; // Reset button state
        }

        // When in forced tach state, allow "+" and "-" buttons to adjust RPM
        if (forceTachometer) {
            // Check for "+" button to increase RPM
            if (canId == plusButtonID) {
                forcedRPM += 1000; // Increase RPM by 1000
                if (forcedRPM > 3000) {
                    forcedRPM = 1000; // Loop back to 1000 after 3000
                }
                sendTachometerRPM(forcedRPM); // Send the updated RPM message
                sendForcedRPMPosition(); // Send CAN message with updated RPM position
            }

            // Check for "-" button to decrease RPM
            if (canId == minusButtonID) {
                forcedRPM -= 1000; // Decrease RPM by 1000
                if (forcedRPM < 1000) {
                    forcedRPM = 3000; // Loop back to 3000 after 1000
                }
                sendTachometerRPM(forcedRPM); // Send the updated RPM message
                sendForcedRPMPosition(); // Send CAN message with updated RPM position
            }
        }
    }

    // Manage RPM sending based on the forced state
    if (forceTachometer) {
        // When forced, maintain the set RPM
        sendTachometerRPM(forcedRPM);
    } else {
        // Send the actual RPM when not forcing
        int actualRPM = readActualRPM(); // Replace with your actual RPM function
        sendTachometerRPM(actualRPM); // Send the actual RPM message
    }
}

// Function to send RPM value via CAN
void sendTachometerRPM(int rpm) {
    // Example CAN message structure
    byte canMessage[8]; // CAN messages can have up to 8 data bytes
    canMessage[0] = rpm & 0xFF; // Lower byte of RPM
    canMessage[1] = (rpm >> 8) & 0xFF; // Higher byte of RPM

    // SavvyCAN message formatting
    uint32_t canId = 0x100; // Replace with your CAN ID
    uint8_t dataLength = 2; // Number of bytes in the message

    // Send CAN message using SavvyCAN style
    CAN.sendMsgBuf(canId, 0, dataLength, canMessage);
    Serial.print("Sent CAN RPM: ");
    Serial.println(rpm);
}

// Function to send forced RPM position when it changes
void sendForcedRPMPosition() {
    if (forcedRPM != previousForcedRPM) {
        // Send forced RPM position if changed
        byte canMessage[8];
        canMessage[0] = forcedRPM & 0xFF; // Lower byte of forced RPM position
        canMessage[1] = (forcedRPM >> 8) & 0xFF; // Higher byte of forced RPM position

        CAN.sendMsgBuf(forcedRPMPositionID, 0, 2, canMessage);
        Serial.print("Sent forced RPM position: ");
        Serial.println(forcedRPM);

        previousForcedRPM = forcedRPM; // Update last sent position
    }
}

// Function to read actual RPM (replace with your logic)
int readActualRPM() {
    // Placeholder function - implement your own logic to read actual RPM
    return 2000; // Example RPM value
}
