#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_CS    18
#define LORA_RST   23
#define LORA_DIO0  26

void setup()
{
    Serial.begin(115200);
    delay(1000);

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa initialization failed");
        while (true);
    }

    Serial.println("BOARD B READY");
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

        Serial.print("Received: ");
        Serial.print(message);

        Serial.print(" | RSSI: ");
        Serial.println(LoRa.packetRssi());

        delay(50);

        // Reply
        LoRa.beginPacket();
        LoRa.print("ACK ");
        LoRa.print(message);
        LoRa.endPacket();

        Serial.print("Replied: ACK ");
        Serial.println(message);
    }
}
