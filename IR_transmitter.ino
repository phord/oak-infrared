#include <Ticker.h>

#define txPin P0

#define FREQUENCY       38000    // 38 kHz
//#define FREQUENCY       10000
#define period          (1000000/(FREQUENCY))
#define half_period     (period/2)
#define us_to_pulses(us) ((us)/(half_period))

int tx_head=0;
int tx_tail=0;
const unsigned tx_queuesize = 1000;
unsigned long tx_signals[tx_queuesize];

void tx_send(unsigned long ton, unsigned long toff) {
  tx_signals[tx_head] = us_to_pulses(ton);
  auto next = (tx_head + 1 ) % tx_queuesize ;
  tx_signals[next++] = us_to_pulses(toff);
  next %= tx_queuesize ;
  noInterrupts();
  tx_head = next;
  interrupts();
}


void tx_carrier()
{
  noInterrupts();
  int head = tx_head;
  int tail = tx_tail;
  interrupts();
  if (head == tail) return;
  
  static int state = 1;  // first pulse is ON; next one is OFF
  static int toggle = 0;

  if (0 == tx_signals[tail]--) {
    // ran out of time for the previous state.  Time for a new one.
    state = !state;
    tail = (tail + 1 ) % tx_queuesize ;

    noInterrupts();
    tx_tail = tail;
    interrupts();
    if (tail == head) return;
  }

  // Go dark on OFF state
  if (!state) toggle = 0;
  digitalWrite(txPin, toggle);     // set pin to the opposite state
  toggle = !toggle;
}

#define SEC 1000000L

void tx_setup() {
  pinMode(txPin, OUTPUT);

  for (int i = 0; i< 200; i++) {
    tx_send((i%3)*600, 600);
  }  
}

static unsigned long prev=0;
void tx_loop() {
  if (tx_head == tx_tail)  { prev=0; return; }
  if (micros()-prev > half_period) {
    prev = micros();
    tx_carrier();
  }
}

