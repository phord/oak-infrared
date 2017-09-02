    int irPin = P2; //IR detector connected to digital 2

    const byte BIT_PER_BLOCK = 32;
     
    void ir_setup() {
      pinMode(irPin, INPUT);
      pinMode(LED_BUILTIN, OUTPUT);
      flash();
      flash();
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
auto prev_time = micros();

int sig=0;
unsigned long signals[1000];

void ir_loop() {
/*        int pulse_len = pulseIn(irPin, HIGH);

        int out = 0;
        if (pulse_len > 4000) bose(2);
        else if (pulse_len > 1000) bose(1);
        else if (pulse_len > 100 ) bose(0);
*/
    int ir = digitalRead(irPin);
    auto now = micros();
    if (sig < 1000 && ir != prev_ir) {
      auto len = now - prev_time;
      if (len < 1000000) signals[sig++] = len;
      prev_ir = ir;
      prev_time = now;
      digitalWrite(LED_BUILTIN, ir);
    }

    if (now - prev_time > 2000000) {
      if (sig && serverClient && serverClient.connected()) {  // send data to Client
        serverClient.print("IR: n=");
        serverClient.println(sig);
        serverClient.print(" ==> ");
        for (int i = 0; i < sig ; i++) {
          serverClient.print(" ");
          serverClient.print(signals[i]);
          if (i<sig-1 && signals[i] > 15000) {
            serverClient.println();
            serverClient.print(" ==> ");
          }
        }
        serverClient.println();
       //serverClient.println(digitalRead(irPin));
      }
      sig = 0;
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


