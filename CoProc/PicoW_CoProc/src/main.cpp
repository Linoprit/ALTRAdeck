#include <Arduino.h>


void ledinitialize() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}
void ledOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}
void ledOff() {
  digitalWrite(LED_BUILTIN, LOW);
}


// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  ledinitialize();
}

// the loop function runs over and over again forever
void loop() {
  Serial.println("LED on");
  ledOn();
  delay(1000);  
                       // wait for a second
  Serial.println("LED off");
  ledOff();
  delay(1000);                       // wait for a second
}