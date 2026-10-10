#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_RST   23
#define LORA_DIO0  26

#define LED_PIN    25

unsigned long lastSend = 0;
int counter = 0;

void flashLED()
{
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
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

        while (true)
        {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(200);
        }
    }

    Serial.println("BOARD A READY");
}

void loop()
{
    // Send ping every 2 seconds
    if (millis() - lastSend >= 2000)
    {
        counter++;

        LoRa.beginPacket();
        LoRa.print("PING ");
        LoRa.print(counter);
        LoRa.endPacket();

        Serial.print("Sent: PING ");
        Serial.println(counter);

        lastSend = millis();
    }

    // Look for reply
    int packetSize = LoRa.parsePacket();

    if (packetSize)
    {
        String message = "";

        while (LoRa.available())
        {
            message += (char)LoRa.read();
        }

        Serial.print("Received: ");
        Serial.print(message);

        Serial.print(" | RSSI: ");
        Serial.println(LoRa.packetRssi());

        flashLED();
    }
}
