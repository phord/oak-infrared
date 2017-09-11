    int irPin = P2; //IR detector connected to digital 2

    const byte BIT_PER_BLOCK = 32;

auto prev_time = micros();
int head=0;
int tail=0;
const unsigned queuesize = 1000;
unsigned long signals[queuesize];

void handleInterrupt()
{
    auto now = micros();
    auto next = (tail + 1 ) % queuesize ;
    auto len = now - prev_time;
    signals[tail] = len;
    tail = next;
    if (next == head) {
      head = (head + 1) % queuesize  ;
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

#define NEXT(i) (((i)+1) % queuesize)

int readBit( int & necHead ) {
  // on-codes should be about 550us.  Mark+space should be either 1100 or 2200.
  auto mark = signals[necHead];
  if (mark > 700 || mark < 500) return -1;
  necHead = NEXT(necHead);

  auto gap = signals[necHead];
  int ret = 0;
//  serverClient.print(" [");
//  serverClient.print(necHead);
//  serverClient.print(",");
//  serverClient.print(mark+gap);
//  serverClient.print(",");
//  serverClient.print(gap);
//  serverClient.print("] ");
  if (mark+gap < 2220 && mark+gap > 1980)     ret = 1;
  else if (mark+gap > 1150 || mark+gap < 950) return -1;  // bad code
  necHead = NEXT(necHead);
  return ret;  
}

unsigned long necDecode()
{
  static int necHead = -1;
  int necTail;

  noInterrupts();
  necTail = tail;
  if (necHead < 0) necHead = head;
  interrupts();
  
  // NEC codes from:  http://www.circuitvalley.com/2013/09/nec-protocol-ir-infrared-remote-control.html
  //    A 9ms leading pulse burst (16 times the pulse burst length used for a logical data bit)
  //    A 4.5ms space
  //    The 8-bit address for the receiving device
  //    The 8-bit logical inverse of the address
  //    The 8-bit command
  //    The 8-bit logical inverse of the command
  //    Final 562.5µs pulse burst to show end of message transmission.
  //    Logical '0' – a 562.5µs pulse burst followed by a 562.5µs space, with a total transmit time of 1.125ms
  //    Logical '1' – a 562.5µs pulse burst followed by a 1.6875ms space, with a total transmit time of 2.25    

  // I find in practice my logical data bits are 551us on the Bose remote I want to emulate.
  // The leading pulse is 8810us.  16 * 551 = 8804, so that matches.  But it's not 9ms; 9000/16=562.5.  It's close, though.
  // Maybe the difference is the IR pulse frequency.  38KHz pulses vs 36KHz pulses.  Nope, that would make it 580us.
  // Maybe I'm reading the signals wrong.  The IR receiver has some slop in the generated pulses.  It seems to run about
  // long on the pulses and makes the spaces shorter. The marks are 10us to 40us too long.  From repeated samples I'm able to
  // come up with the 551 number.
  // 38KHz pulses = 26.3158us long
  // 36KHz pulses = 27.7778us long
  //
  // 551 Matches most closely with 38Hz, being about 21 * 26.3158 = 552.63
  // Let's go with that for now.

  int len = (necTail - necHead + queuesize) % queuesize;

  // Find a start mark
  while (len > 2) {
    // advance queue until we find start code
    if ( signals[necHead] > 8700 && signals[necHead] < 8900 ) {
      int next = NEXT(necHead);
      if ( signals[next] > 4200 && signals[next] < 4500 ) {
        break;
      }
    }
    --len;
    necHead = NEXT(necHead);
  }

  // NEC codes are 67 samples long, (8*4 + start + end/2) * 2
  // If we don't have 67 samples yet, bail.
  if (len < 67) return 0;
 
  necHead = NEXT(necHead+1);
  necTail = NEXT(necHead+64);

  unsigned long code = 0;
  unsigned long mask = 0x80000000;
  while ( mask ) {
    int b = readBit(necHead);
  
//  serverClient.print(" [");
//  serverClient.print(necHead);
//  serverClient.print(",");
//  serverClient.print(b);
//  serverClient.print(",");
//  serverClient.print(mask);
//  serverClient.println("] ");
    if (b == -1) return 0;
    if (b) code |= mask;
    mask /= 2;
  }

  // read stop bit (high mark only)
  auto mark = signals[necHead];
//  serverClient.print(" [ mark=");
//  serverClient.print(mark);
//  serverClient.print(",");
//  serverClient.print(code);
//  serverClient.println("] ");
  if (mark > 700 || mark < 500) return 0;
  necHead = NEXT(necHead);

  //TODO: verify addr and cmd match their inverses
  return code;
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
      head = (head + 1) % queuesize ;
      interrupts();
      
//      serverClient.print(" ");
//      serverClient.print(sig);
//      if (sig > 15000) {
//        serverClient.println();
//        serverClient.print(" => ");
//      }
    }
    for (int code ; code = necDecode(); ) {
      char str[30];
      sprintf(str, "  NEC=%08x  ", code);
      serverClient.println(str);
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


