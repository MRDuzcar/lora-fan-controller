#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_RST   23
#define LORA_DIO0  26

#define LED_PIN    25

void blinkLED(int times)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(200);

        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa failed");
        while (true);
    }

    Serial.println("RECEIVER READY");
}

void loop()
{
    int packetSize = LoRa.parsePacket();

    if (packetSize)
    {
        String message = "";

        while (LoRa.available())
        {
            message += (char)LoRa.read();
        }

        int buttonNumber = message.toInt();

        Serial.print("Received button: ");
        Serial.println(buttonNumber);

        if (buttonNumber >= 1 && buttonNumber <= 3)
        {
            blinkLED(buttonNumber);
        }
    }
}
