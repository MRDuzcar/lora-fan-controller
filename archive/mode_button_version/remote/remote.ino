#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
// OLED
// =============================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =============================
// Controls
// =============================
const int modeButton  = 14;
const int downButton  = 12;
const int upButton    = 13;
const int powerButton = 0;
const int potPin      = 36;

// =============================
// Variables
// =============================
bool potMode = true;
bool fanPowerOn = false;

int targetRPM = 0;
int feedbackRPM = 0;

bool lastModeState  = HIGH;
bool lastDownState  = HIGH;
bool lastUpState    = HIGH;
bool lastPowerState = HIGH;

unsigned long lastPotSend = 0;

// =============================
// Update OLED
// =============================
void updateDisplay()
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);

    display.print("MODE: ");

    if (potMode)
        display.print("POT");
    else
        display.print("BUTTON");

    display.print("  PWR:");

    if (fanPowerOn)
        display.println("ON");
    else
        display.println("OFF");

    display.setTextSize(2);

    display.setCursor(0, 18);
    display.print("SET:");
    display.print(targetRPM);

    display.setCursor(0, 42);
    display.print("RPM:");
    display.print(feedbackRPM);

    display.display();
}

// =============================
// Send target RPM
// =============================
void sendRPM()
{
    LoRa.beginPacket();
    LoRa.print("RPM=");
    LoRa.print(targetRPM);
    LoRa.endPacket();

    Serial.print("SET RPM: ");
    Serial.println(targetRPM);

    updateDisplay();
}

// =============================
// Send power command
// =============================
void sendPower()
{
    LoRa.beginPacket();
    LoRa.print("POWER=");

    if (fanPowerOn)
        LoRa.print("1");
    else
        LoRa.print("0");

    LoRa.endPacket();

    Serial.print("FAN POWER: ");

    if (fanPowerOn)
        Serial.println("ON");
    else
        Serial.println("OFF");

    updateDisplay();
}

// =============================
// Setup
// =============================
void setup()
{
    Serial.begin(115200);

    pinMode(modeButton, INPUT_PULLUP);
    pinMode(downButton, INPUT_PULLUP);
    pinMode(upButton, INPUT_PULLUP);
    pinMode(powerButton, INPUT_PULLUP);

    pinMode(potPin, INPUT);

    // OLED
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("OLED failed");

        while (true);
    }

    display.clearDisplay();
    display.display();

    // LoRa
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa failed");

        while (true);
    }

    Serial.println("REMOTE READY");

    updateDisplay();
}

// =============================
// Main loop
// =============================
void loop()
{
    bool modeState  = digitalRead(modeButton);
    bool downState  = digitalRead(downButton);
    bool upState    = digitalRead(upButton);
    bool powerState = digitalRead(powerButton);

    // =========================
    // POWER BUTTON
    // =========================
    if (lastPowerState == HIGH && powerState == LOW)
    {
        fanPowerOn = !fanPowerOn;

        sendPower();

        delay(50);
    }

    lastPowerState = powerState;

    // =========================
    // MODE BUTTON
    // =========================
    if (lastModeState == HIGH && modeState == LOW)
    {
        potMode = !potMode;

        Serial.print("MODE: ");

        if (potMode)
            Serial.println("POT");
        else
            Serial.println("BUTTON");

        updateDisplay();

        delay(50);
    }

    lastModeState = modeState;

    // =========================
    // POT MODE
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
    // BUTTON MODE
    // =========================
    else
    {
        if (lastUpState == HIGH && upState == LOW)
        {
            targetRPM += 100;

            if (targetRPM > 2700)
                targetRPM = 2700;

            sendRPM();

            delay(50);
        }

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

    // =========================
    // RECEIVE FAN FEEDBACK
    // =========================
    int packetSize = LoRa.parsePacket();

    if (packetSize)
    {
        String message = "";

        while (LoRa.available())
        {
            message += (char)LoRa.read();
        }

        if (message.startsWith("ACTUAL="))
        {
            feedbackRPM =
                message.substring(7).toInt();

            Serial.print("SET: ");
            Serial.print(targetRPM);

            Serial.print(" | FEEDBACK: ");
            Serial.println(feedbackRPM);

            updateDisplay();
        }
    }
}
