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

void sendButton(int buttonNumber)
{
    LoRa.beginPacket();
    LoRa.print(buttonNumber);
    LoRa.endPacket();

    Serial.print("Sent button: ");
    Serial.println(buttonNumber);

    delay(250);   // simple debounce
}

void setup()
{
    Serial.begin(115200);

    pinMode(modeButton, INPUT_PULLUP);
    pinMode(downButton, INPUT_PULLUP);
    pinMode(upButton, INPUT_PULLUP);

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa failed");
        while (true);
    }

    Serial.println("TRANSMITTER READY");
}

void loop()
{
    if (digitalRead(modeButton) == LOW)
    {
        sendButton(1);

        while (digitalRead(modeButton) == LOW);
    }

    if (digitalRead(downButton) == LOW)
    {
        sendButton(2);

        while (digitalRead(downButton) == LOW);
    }

    if (digitalRead(upButton) == LOW)
    {
        sendButton(3);

        while (digitalRead(upButton) == LOW);
    }
}
