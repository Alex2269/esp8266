
uint8_t triggerPin = 13;
uint8_t echoPin = 12;

void init_HCSR04(uint8_t triggerPin, uint8_t echoPin)
{
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);
}

double measure_HCSR04(void)
{
    // Make sure that trigger pin is LOW.
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    // Hold trigger for 10 microseconds, which is signal for sensor to measure distance.
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    // Measure the length of echo signal, which is equal to the time needed for sound to go there and back.
    unsigned long durationMicroSec = pulseIn(echoPin, HIGH);
    double distanceCm = durationMicroSec / 2.0 * 0.0343;
    if (distanceCm == 0 || distanceCm > 400) {
        return -1.0 ;
    } else {
        return distanceCm;
    }
}

void setup () {
    init_HCSR04(triggerPin, echoPin);
    Serial.begin(9600);
}

void loop () {
    Serial.println(measure_HCSR04());
    delay(500);
}
