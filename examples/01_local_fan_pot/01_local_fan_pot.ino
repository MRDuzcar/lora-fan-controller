const int tachPin = 4;
const int pwmPin  = 5;
const int potPin  = 3;

volatile unsigned long pulseCount = 0;
unsigned long lastTime = 0;

void IRAM_ATTR tachISR()
{
    pulseCount++;
}

void setup()
{
    Serial.begin(115200);

    pinMode(tachPin, INPUT);
    pinMode(potPin, INPUT);

    attachInterrupt(digitalPinToInterrupt(tachPin), tachISR, FALLING);

    ledcAttach(pwmPin, 25000, 8);
}

void loop()
{
    int potValue = analogRead(potPin);

    int fanPercent = map(potValue, 0, 4095, 0, 100);

    int fanDuty = map(fanPercent, 0, 100, 0, 255);

    // 2N2222 inverts the fan PWM signal
    int transistorDuty = 255 - fanDuty;

    ledcWrite(pwmPin, transistorDuty);

    if (millis() - lastTime >= 1000)
    {
        noInterrupts();

        unsigned long pulses = pulseCount;
        pulseCount = 0;

        interrupts();

        unsigned long rpm = pulses * 30;

        Serial.print("Pot: ");
        Serial.print(potValue);

        Serial.print(" | Fan: ");
        Serial.print(fanPercent);
        Serial.print("%");

        Serial.print(" | RPM: ");
        Serial.println(rpm);

        lastTime += 1000;
    }
}
