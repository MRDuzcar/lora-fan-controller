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
// Fan pins
// =============================
const int tachPin   = 4;
const int pwmPin    = 12;
const int mosfetPin = 13;

// =============================
// Variables
// =============================
volatile unsigned long pulseCount = 0;

unsigned long lastRPMTime = 0;

int targetRPM = 0;
int actualRPM = 0;

bool fanPowerOn = false;

// =============================
// Tach interrupt
// =============================
void IRAM_ATTR tachISR()
{
    pulseCount++;
}

// =============================
// Hard power control
// =============================
void setFanPower(bool state)
{
    fanPowerOn = state;

    // Reset tach counter whenever
    // power state changes
    noInterrupts();
    pulseCount = 0;
    interrupts();

    if (fanPowerOn)
    {
        // Turn fan power ON
        digitalWrite(mosfetPin, HIGH);

        // Enable tach pull-up only
        // while the fan is powered
        pinMode(tachPin, INPUT_PULLUP);

        Serial.println("FAN POWER: ON");
    }
    else
    {
        // Disable tach pull-up FIRST
        // so 3.3V cannot backfeed
        // into the unpowered fan
        pinMode(tachPin, INPUT);

        // Turn fan power OFF
        digitalWrite(mosfetPin, LOW);

        actualRPM = 0;

        Serial.println("FAN POWER: OFF");
    }
}

// =============================
// Set fan PWM
// =============================
void setFanRPM(int requestedRPM)
{
    requestedRPM = constrain(requestedRPM, 0, 2700);

    targetRPM = requestedRPM;

    int fanDuty = map(targetRPM, 0, 2700, 0, 255);

    /*
       The 2N2222 inverts the fan PWM signal.

       Higher requested fan duty
       therefore requires lower GPIO duty.
    */

    int transistorDuty =
        255 - fanDuty;

    ledcWrite(pwmPin, transistorDuty);

    Serial.print("Target RPM: ");
    Serial.print(targetRPM);

    Serial.print(" | Fan PWM: ");
    Serial.println(fanDuty);
}

// =============================
// Setup
// =============================
void setup()
{
    Serial.begin(115200);

    // =========================
    // MOSFET POWER
    // =========================
    pinMode(mosfetPin, OUTPUT);

    // Safe startup:
    // fan power OFF
    digitalWrite(mosfetPin, LOW);

    fanPowerOn = false;

    // =========================
    // TACHOMETER
    // =========================

    /*
       IMPORTANT:
       Do NOT use an external pull-up
       from tach to 3.3V for this test.

       Fan OFF:
       GPIO stays high impedance.

       Fan ON:
       ESP32 internal pull-up is enabled.
    */

    pinMode(tachPin, INPUT);

    attachInterrupt(digitalPinToInterrupt(tachPin), tachISR, FALLING);

    // =========================
    // FAN PWM
    // =========================
    ledcAttach(pwmPin, 25000, 8);

    // Initial PWM command
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

    Serial.println("Fan starts powered OFF.");
}

// =============================
// Main loop
// =============================
void loop()
{
    // =========================
    // RECEIVE LoRa PACKET
    // =========================
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

            receivedRPM =
                constrain(receivedRPM, 0, 2700);

            setFanRPM(receivedRPM);
        }

        // =====================
        // POWER ON
        // =====================
        else if (message == "POWER=1")
        {
            setFanPower(true);
        }

        // =====================
        // POWER OFF
        // =====================
        else if (message == "POWER=0")
        {
            setFanPower(false);
        }
    }

    // =========================
    // RPM MEASUREMENT
    // =========================
    if (millis() - lastRPMTime >= 1000)
    {
        noInterrupts();

        unsigned long pulses =
            pulseCount;

        pulseCount = 0;

        interrupts();

        if (fanPowerOn)
        {
            /*
               Assuming:
               2 tach pulses
               per revolution

               1 second sample:

               RPM =
               pulses × 60 / 2
               = pulses × 30
            */

            actualRPM =
                pulses * 30;
        }
        else
        {
            actualRPM = 0;
        }

        // =====================
        // SERIAL OUTPUT
        // =====================
        Serial.print("POWER: ");

        if (fanPowerOn)
            Serial.print("ON");
        else
            Serial.print("OFF");

        Serial.print(" | SET: ");
        Serial.print(targetRPM);

        Serial.print(" | ACTUAL: ");
        Serial.print(actualRPM);

        Serial.println(" RPM");

        // =====================
        // SEND RPM FEEDBACK
        // =====================
        LoRa.beginPacket();

        LoRa.print("ACTUAL=");
        LoRa.print(actualRPM);

        LoRa.endPacket();

        lastRPMTime += 1000;
    }
}
