#include <SPI.h>
#include <LoRa.h>

// =============================
// LoRa pins
// =============================
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_RST   23
#define LORA_DIO0  26

// =============================
// Fan/control pins
// =============================
const int pwmPin    = 12;
const int mosfetPin = 13;
const int ledPin    = 25;

// =============================
// Variables
// =============================
int targetRPM = 0;

// This represents the
// physical power-button state
bool masterEnabled = false;

// =============================
// Update MOSFET + LED
// =============================
void updatePowerState()
{
    /*
       LED follows ONLY the
       master power button.
    */

    digitalWrite(ledPin, masterEnabled ? HIGH : LOW);

    /*
       MOSFET requires BOTH:

       1. Master enabled
       2. RPM >= 500
    */

    bool fanPowerOn =
        masterEnabled &&
        targetRPM >= 500;

    digitalWrite(mosfetPin, fanPowerOn ? HIGH : LOW);

    Serial.print("MASTER: ");

    Serial.print(masterEnabled ? "ON" : "OFF");

    Serial.print(" | TARGET: ");
    Serial.print(targetRPM);

    Serial.print(" | MOSFET: ");

    Serial.println(fanPowerOn ? "ON" : "OFF");
}

// =============================
// Set PWM
// =============================
void setFanRPM(int requestedRPM)
{
    // Only allow:
    // 0 or 500-2700

    if (requestedRPM > 0 && requestedRPM < 500)
    {
        requestedRPM = 500;
    }

    requestedRPM =
        constrain(requestedRPM, 0, 2700);

    targetRPM =
        requestedRPM;

    // =========================
    // RPM → duty
    // =========================
    int fanDuty;

    if (targetRPM == 0)
    {
        fanDuty = 0;
    }
    else
    {
        fanDuty =
            map(targetRPM, 500, 2700, 47, 255);
    }

    /*
       Existing 2N2222 PWM
       stage inverts the signal.
    */

    int transistorDuty =
        255 - fanDuty;

    ledcWrite(pwmPin, transistorDuty);

    Serial.print("TARGET RPM: ");
    Serial.print(targetRPM);

    Serial.print(" | PWM: ");
    Serial.println(fanDuty);

    // RPM may have crossed
    // 0/500 threshold
    updatePowerState();
}

// =============================
// Setup
// =============================
void setup()
{
    Serial.begin(115200);

    // =========================
    // MOSFET
    // =========================
    pinMode(mosfetPin, OUTPUT);

    digitalWrite(mosfetPin, LOW);

    // =========================
    // STAND LED
    // =========================
    pinMode(ledPin, OUTPUT);

    digitalWrite(ledPin, LOW);

    // =========================
    // FAN PWM
    // =========================
    ledcAttach(pwmPin, 25000, 8);

    setFanRPM(0);

    // =========================
    // LoRa
    // =========================
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa initialization failed");

        while (true);
    }

    Serial.println();
    Serial.println("FAN RECEIVER READY");

    Serial.println("Master starts OFF.");
}

// =============================
// Main loop
// =============================
void loop()
{
    int packetSize =
        LoRa.parsePacket();

    if (packetSize)
    {
        String message = "";

        while (LoRa.available())
        {
            message +=
                (char)LoRa.read();
        }

        Serial.print("Received: ");
        Serial.println(message);

        // =====================
        // RPM COMMAND
        // =====================
        if (message.startsWith("RPM="))
        {
            int receivedRPM =
                message
                .substring(4)
                .toInt();

            setFanRPM(receivedRPM);
        }

        // =====================
        // MASTER ENABLE ON
        // =====================
        else if (message == "ENABLE=1")
        {
            masterEnabled = true;

            updatePowerState();
        }

        // =====================
        // MASTER ENABLE OFF
        // =====================
        else if (message == "ENABLE=0")
        {
            masterEnabled = false;

            updatePowerState();
        }
    }
}
