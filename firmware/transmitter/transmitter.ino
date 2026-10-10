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
const int potLedPin    = 25;
const int buttonLedPin = 4;

// =============================
// State
// =============================
bool potMode = true;
bool masterEnabled = false;

int targetRPM = 0;

// Last RPM actually sent over LoRa
int lastSentRPM = -1;

bool lastModeState  = HIGH;
bool lastDownState  = HIGH;
bool lastUpState    = HIGH;
bool lastPowerState = HIGH;

// =============================
// Timing
// =============================
unsigned long lastPotCheck = 0;
unsigned long lastPotSend  = 0;
unsigned long lastLoRaSend = 0;

const unsigned long POT_CHECK_INTERVAL = 100;
const unsigned long POT_SEND_INTERVAL  = 600;
const unsigned long LORA_QUIET_TIME    = 250;

// =============================
// OLED
// Only displays target RPM
// =============================
void updateDisplay()
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Top status line
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print("MODE: ");
    display.println(potMode ? "POT" : "BUTTON");

    display.setCursor(0, 10);
    display.print("POWER: ");
    display.println(masterEnabled ? "ON" : "OFF");

    // Target RPM
    display.setTextSize(2);

    display.setCursor(0, 28);
    display.println("TARGET");

    display.setCursor(0, 48);
    display.print(targetRPM);
    display.print(" RPM");

    display.display();
}

// =============================
// Send RPM
// =============================
void sendRPM()
{
    LoRa.beginPacket();

    LoRa.print("RPM=");
    LoRa.print(targetRPM);

    LoRa.endPacket();

    lastSentRPM = targetRPM;
    lastLoRaSend = millis();
    lastPotSend = millis();

    Serial.print("RPM SENT: ");
    Serial.println(targetRPM);

    updateDisplay();
}

// =============================
// Enable corresponding mode LED
// =============================
void updateModeLEDs()
{
    digitalWrite(potLedPin, potMode ? HIGH : LOW);
    digitalWrite(buttonLedPin, potMode ? LOW : HIGH);
}

// =============================
// Send master ENABLE
// =============================
void sendEnable()
{
    LoRa.beginPacket();

    LoRa.print("ENABLE=");
    LoRa.print(masterEnabled ? "1" : "0");

    LoRa.endPacket();

    lastLoRaSend = millis();

    Serial.print("MASTER: ");
    Serial.println(masterEnabled ? "ENABLED" : "DISABLED");

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

    // =========================
    // OLED
    // =========================
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("OLED failed");

        while (true);
    }

    display.clearDisplay();
    display.display();

    // =========================
    // LoRa
    // =========================
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa failed");

        while (true);
    }

    Serial.println("REMOTE READY");

    masterEnabled = false;
    targetRPM = 0;

    updateDisplay();
    pinMode(potLedPin, OUTPUT);
    pinMode(buttonLedPin, OUTPUT);

    updateModeLEDs();

    // Give receiver initial state
    sendEnable();

    delay(300);

    sendRPM();
}

// =============================
// Main loop
// =============================
void loop()
{
    bool modeState =
        digitalRead(modeButton);

    bool downState =
        digitalRead(downButton);

    bool upState =
        digitalRead(upButton);

    bool powerState =
        digitalRead(powerButton);

    // =========================
    // POWER BUTTON
    // =========================
    if (lastPowerState == HIGH && powerState == LOW)
    {
        masterEnabled =
            !masterEnabled;

        sendEnable();

        delay(50);
    }

    lastPowerState =
        powerState;

    // =========================
    // MODE BUTTON
    // Local only - NO LoRa packet
    // =========================
    if (lastModeState == HIGH && modeState == LOW)
    {
        potMode =
            !potMode;

        updateModeLEDs();

        Serial.print("MODE: ");
        Serial.println(potMode ? "POT" : "BUTTON");

        updateDisplay();

        delay(50);
    }

    lastModeState =
        modeState;

    // =========================
    // POT MODE
    // =========================
    if (potMode)
    {
        if (millis() - lastPotCheck >= POT_CHECK_INTERVAL)
        {
            lastPotCheck = millis();

            int rawPot =
                analogRead(potPin);

            int newRPM;

            // Bottom of pot = 0
            if (rawPot < 150)
            {
                newRPM = 0;
            }
            else
            {
                newRPM =
                    map(rawPot, 150, 4095, 500, 2700);

                newRPM =
                    constrain(newRPM, 500, 2700);

                // Round to nearest 100 RPM
                newRPM =
                    ((newRPM + 50) / 100)
                    * 100;

                if (newRPM > 2700)
                {
                    newRPM = 2700;
                }
            }

            targetRPM = newRPM;

            updateDisplay();

            // Only send if RPM actually changed
            // AND enough time has passed.
            if (
                targetRPM != lastSentRPM &&
                millis() - lastPotSend >=
                    POT_SEND_INTERVAL &&
                millis() - lastLoRaSend >=
                    LORA_QUIET_TIME
            )
            {
                sendRPM();
            }
        }
    }

    // =========================
    // BUTTON MODE
    // =========================
    else
    {
        // ---------------------
        // UP
        // ---------------------
        if (lastUpState == HIGH && upState == LOW)
        {
            if (targetRPM == 0)
            {
                targetRPM = 500;
            }
            else
            {
                targetRPM += 100;

                if (targetRPM > 2700)
                {
                    targetRPM = 2700;
                }
            }

            sendRPM();

            delay(50);
        }

        // ---------------------
        // DOWN
        // ---------------------
        if (lastDownState == HIGH && downState == LOW)
        {
            if (targetRPM <= 500)
            {
                targetRPM = 0;
            }
            else
            {
                targetRPM -= 100;
            }

            sendRPM();

            delay(50);
        }
    }

    lastUpState =
        upState;

    lastDownState =
        downState;
}
