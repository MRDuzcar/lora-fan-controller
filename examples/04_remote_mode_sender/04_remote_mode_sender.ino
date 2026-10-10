#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_RST   23
#define LORA_DIO0  26

const int modeButton = 14;
const int downButton = 12;
const int upButton   = 13;
const int potPin     = 36;

bool potMode = true;

int targetRPM = 0;

bool lastModeState = HIGH;
bool lastDownState = HIGH;
bool lastUpState   = HIGH;

unsigned long lastPotSend = 0;

void sendRPM()
{
    LoRa.beginPacket();
    LoRa.print("RPM=");
    LoRa.print(targetRPM);
    LoRa.endPacket();

    Serial.print("Mode: ");
    Serial.print(potMode ? "POT" : "BUTTON");

    Serial.print(" | Target RPM: ");
    Serial.println(targetRPM);
}

void setup()
{
    Serial.begin(115200);

    pinMode(modeButton, INPUT_PULLUP);
    pinMode(downButton, INPUT_PULLUP);
    pinMode(upButton, INPUT_PULLUP);

    pinMode(potPin, INPUT);

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa failed");

        while (true);
    }

    Serial.println("REMOTE READY");
}

void loop()
{
    bool modeState = digitalRead(modeButton);
    bool downState = digitalRead(downButton);
    bool upState   = digitalRead(upButton);

    // -------------------------
    // MODE BUTTON
    // -------------------------
    if (lastModeState == HIGH && modeState == LOW)
    {
        potMode = !potMode;

        Serial.print("Switched to: ");

        if (potMode)
            Serial.println("POT MODE");
        else
            Serial.println("BUTTON MODE");

        delay(50);
    }

    lastModeState = modeState;

    // =========================
    // POTENTIOMETER MODE
    // =========================
    if (potMode)
    {
        if (millis() - lastPotSend >= 200)
        {
            int rawPot = analogRead(potPin);

            targetRPM = map(rawPot, 0, 4095, 0, 2700);

            sendRPM();

            lastPotSend = millis();
        }
    }

    // =========================
    // BUTTON / INCREMENT MODE
    // =========================
    else
    {
        // UP pressed
        if (lastUpState == HIGH && upState == LOW)
        {
            targetRPM += 100;

            if (targetRPM > 2700)
                targetRPM = 2700;

            sendRPM();

            delay(50);
        }

        // DOWN pressed
        if (lastDownState == HIGH && downState == LOW)
        {
            targetRPM -= 100;

            if (targetRPM < 0)
                targetRPM = 0;

            sendRPM();

            delay(50);
        }
    }

    lastUpState = upState;
    lastDownState = downState;
}
