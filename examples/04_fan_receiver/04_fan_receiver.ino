#include <SPI.h>
#include <LoRa.h>

// -----------------------------
// LoRa pins
// -----------------------------
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_RST   23
#define LORA_DIO0  26

// -----------------------------
// Fan pins
// -----------------------------
const int tachPin = 4;
const int pwmPin  = 12;

// -----------------------------
// Fan variables
// -----------------------------
volatile unsigned long pulseCount = 0;

unsigned long lastRPMTime = 0;

int targetRPM = 0;
int actualRPM = 0;

// -----------------------------
// Tach interrupt
// -----------------------------
void IRAM_ATTR tachISR()
{
    pulseCount++;
}

// -----------------------------
// Set fan speed
// -----------------------------
void setFanRPM(int requestedRPM)
{
    requestedRPM = constrain(requestedRPM, 0, 2700);

    // Convert requested RPM
    // to 0–255 fan duty
    int fanDuty = map(requestedRPM, 0, 2700, 0, 255);

    /*
       2N2222 inverts PWM:

       GPIO HIGH
       → transistor ON
       → fan PWM LOW

       GPIO LOW
       → transistor OFF
       → fan PWM HIGH

       Therefore invert duty.
    */

    int transistorDuty =
        255 - fanDuty;

    ledcWrite(pwmPin, transistorDuty);

    Serial.print("Target: ");
    Serial.print(requestedRPM);

    Serial.print(" RPM | PWM: ");
    Serial.println(fanDuty);
}

// -----------------------------
// Setup
// -----------------------------
void setup()
{
    Serial.begin(115200);

    // Tachometer
    pinMode(tachPin, INPUT);

    attachInterrupt(digitalPinToInterrupt(tachPin), tachISR, FALLING);

    // Fan PWM
    // 25 kHz
    // 8-bit resolution
    ledcAttach(pwmPin, 25000, 8);

    // Start at zero command
    setFanRPM(0);

    // -------------------------
    // LoRa
    // -------------------------

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa failed");

        while (true);
    }

    Serial.println("FAN RECEIVER READY");
}

// -----------------------------
// Main loop
// -----------------------------
void loop()
{
    // =========================
    // RECEIVE TARGET RPM
    // =========================

    int packetSize = LoRa.parsePacket();

    if (packetSize)
    {
        String message = "";

        while (LoRa.available())
        {
            message += (char)LoRa.read();
        }

        if (message.startsWith("RPM="))
        {
            targetRPM =
                message.substring(4).toInt();

            targetRPM = constrain(targetRPM, 0, 2700);

            Serial.print("Received target: ");
            Serial.println(targetRPM);

            setFanRPM(targetRPM);
        }
    }

    // =========================
    // MEASURE ACTUAL RPM
    // =========================

    if (millis() - lastRPMTime >= 1000)
    {
        noInterrupts();

        unsigned long pulses =
            pulseCount;

        pulseCount = 0;

        interrupts();

        // Fan = 2 tach pulses/revolution
        actualRPM = pulses * 30;

        Serial.print("Target: ");
        Serial.print(targetRPM);

        Serial.print(" | Actual: ");
        Serial.print(actualRPM);

        Serial.println(" RPM");

        // =====================
        // SEND RPM BACK
        // =====================

        LoRa.beginPacket();

        LoRa.print("ACTUAL=");
        LoRa.print(actualRPM);

        LoRa.endPacket();

        lastRPMTime += 1000;
    }
}
