// Enable debug prints to serial monitor
#define MY_DEBUG

#include <SPI.h>
#include "options.h"
#include <MySensors.h>

#define CHILD_ID 0

#define AC_PIN 3
#define LED_PIN A5

volatile int counter = 0;
volatile int ac_status = 0;
volatile unsigned long ac_timer;

MyMessage msg(CHILD_ID, V_LOCK_STATUS);

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("AJAX binding", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  present(CHILD_ID, S_LOCK);
}

void setup() {
  // put your setup code here, to run once:

  pinMode(AC_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(AC_PIN), acCheck, CHANGE);
  pinMode(LED_PIN, OUTPUT);

  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

void acCheck()
{
  ac_timer = millis();
  if (ac_status == 0) {
    // AC ON state
    ac_status = 1;
  }
}

ISR(TIMER1_COMPA_vect) { //timer1 interrupt 1Hz toggles status

  unsigned long timer = millis();
  if (ac_status > 0) {
    if (timer > ac_timer + 500) {
      // AC OFF
      ac_status = 0;
      ac_timer = 0;
      sendCode(false);
    } else if (ac_status == 1) {
      sendCode(true);
      ac_status = 2;
    } else if (ac_status == 2) {
      if (counter++ > 3) {
        digitalWrite(LED_PIN, HIGH);
        counter = 0;
      } else {
        digitalWrite(LED_PIN, LOW);
      }
    }
  } else if (++counter > 1) {
    digitalWrite(LED_PIN, HIGH);
    counter = 0;
  } else {
    digitalWrite(LED_PIN, LOW);
  }

} 

void receive(const MyMessage &message)
{
    // We only expect one type of message from controller. But we better check anyway.
    if (message.type==V_LOCK_STATUS) {
        
        // JUST IGNORE
    }
}

void loop()
{
  // nothing todo
}

void sendCode(bool state) {
  Serial.println(state ? "ON" : "OFF");
  // The LED will be turned on to create a visual signal transmission indicator.
  digitalWrite(LED_PIN, HIGH);

  send(msg.set(state));
   //Turn the LED off after the code has been transmitted.
  digitalWrite(LED_PIN, LOW); 

  
}
 
