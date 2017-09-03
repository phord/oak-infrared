    int irPin = P2; //IR detector connected to digital 2

    const byte BIT_PER_BLOCK = 32;

auto prev_time = micros();
int head=0;
int tail=0;
unsigned long signals[1000];

void handleInterrupt()
{
    auto now = micros();
    auto next = (tail + 1 ) % 1000;
    auto len = now - prev_time;
    signals[tail] = len;
    tail = next;
    if (next == head) {
      head = (head + 1) % 1000 ;
    }
    prev_time = now;
    digitalWrite(LED_BUILTIN, digitalRead(irPin));
}

void ir_setup() {
      pinMode(irPin, INPUT);
      pinMode(LED_BUILTIN, OUTPUT);
      flash();
      flash();
      attachInterrupt(irPin, handleInterrupt, CHANGE);
}

  void flash() {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(50);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(50);                       // wait for a second
  }

typedef enum {
  START,
  ADDRESS,
  ADDRESS_VERIFY,
  COMMAND,
  COMMAND_VERIFY,
  END
} State;

State state = END;

int width;
int addr;
int cmd;
unsigned long acc;

bool accum(int b) {
  acc *= 2;
  acc += b;
}

bool verify() {
  Serial.print("Verify: ");
  Serial.println(acc);
  int x = acc ^ 127;
  return (x&127 == (acc >> 7));
}

void print_acc() {
  char s[20];
  sprintf(s, " # %02x-%02x %02x-%02x# ", acc>>24, (acc>>16)&255, (acc>>8)&255, (acc)&255);
  Serial.print(s);
}

void bose(int b) {
  if ( b > 1 ) {
    Serial.println(b);
    state = START;
    width = 32;
    acc = 0;
    return;
  }
  Serial.print(b);

  accum(b);
  if (--width) return;
  print_acc();

  return;
  
  switch (state) {
    case START:
      if (acc != 2) state = END;
      acc = 0;
      break;
    
    case ADDRESS:
      break;
      
    case ADDRESS_VERIFY:
      print_acc();
      //if (!verify()) state = END;
      //else           addr = acc >> 7;
      acc = 0;
      break;
    
    case COMMAND:
      break;
    
    case COMMAND_VERIFY:
      print_acc();
      if (!verify()) state = END;
      else           cmd = acc >> 7;
      acc = 0;
      break;

    case END:
    default:
      Serial.println("END");
      width=1;
      return;
  }
  if (state < END ) {
    state = State(int(state)+1);
  }
}

int prev_ir = LOW;

void ir_loop() {
/*        int pulse_len = pulseIn(irPin, HIGH);

        int out = 0;
        if (pulse_len > 4000) bose(2);
        else if (pulse_len > 1000) bose(1);
        else if (pulse_len > 100 ) bose(0);
*/
  if (serverClient && serverClient.connected()) {  // send data to Client
    while( head != tail ) {
      noInterrupts();
      auto sig = signals[head];
      head = (head + 1) % 1000;
      interrupts();
      
      serverClient.print(" ");
      serverClient.print(sig);
      if (sig > 15000) {
        serverClient.println();
        serverClient.print(" ==> ");
      }
    }
  }
}

void loopy() {

      while ( !digitalRead(irPin) ) {
        // Do idle stuff here   
      }

      int data[BIT_PER_BLOCK];
      int i;  

     
      for(i = 0 ; i < BIT_PER_BLOCK ; i++) {
        //Start measuring bits, I only want high pulses 
        //you may want to use LOW pulse --> pulseIn(irPin, LOW)
        data[i] = pulseIn(irPin, HIGH);   
      }
     
      delay(100);
      for(i = 0 ; i < BIT_PER_BLOCK ; i++) {
        Serial.println(data[i]);
      }  
      Serial.println("=========");
    }


