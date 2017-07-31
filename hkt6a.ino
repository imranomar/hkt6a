////////////////////// PPM CONFIGURATION//////////////////////////
#define channel_number 5  //set the number of channels
#define sigPin 2  //set PPM signal output pin on the arduino
#define PPM_FrLen 27000  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length
//////////////////////////////////////////////////////////////////

int ppm[channel_number];

//The pulseIn Function
byte PWM_PIN1 = 3; //throttle 1068 to 1886 <- read values from reciever into arduino as max and min ranges
byte PWM_PIN2 = 5; //right when moved up and down 1090 to 1896 <- read values from reciever into arduino as max and min ranges
byte PWM_PIN3 = 6; //right when moved left and right 1075 to 2014 <- read values from reciever into arduino as max and min ranges
byte PWM_PIN4 = 9; //left when moved left and right 1024 to 1826 <- read values from reciever into arduino as max and min ranges
//byte PWM_PIN8 = 8; //lone of the switches

// The sizeof this struct should not exceed 32 bytes
struct MyData {
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;
  //  byte aux;
};

MyData data;

void resetData()
{
  // 'safe' values to use when no radio input is detected
  data.throttle = 1;
  data.yaw = 127;
  data.pitch = 127;
  data.roll = 127;

  setPPMValuesFromData();
}

void setPPMValuesFromData()
{
  ppm[0] = map(data.throttle, 0, 255, 1000, 2000);
  ppm[1] = map(data.yaw,      0, 255, 1000, 2000);
  ppm[2] = map(data.pitch,    0, 255, 1000, 2000);
  ppm[3] = map(data.roll,     0, 255, 1000, 2000);
  ppm[4] = 2000;
}

/**************************************************/

void setupPPM() {
  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, 0);  //set the PPM signal pin to the default state (off)

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register (not very important, sets the timeout for the first interrupt)
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

void setup()
{
  Serial.begin(115200);
  resetData();
  setupPPM();
}

/**************************************************/

unsigned long lastRecvTime = 0;

byte minmax(int x, int mn, int mx) {
  if (x < mn) return mn;
  else if (x > mx) return mx;
  else return x;
}

void recvData()
{
  int t = pulseIn(PWM_PIN1, HIGH);
  int r = pulseIn(PWM_PIN3, HIGH);
  int p = pulseIn(PWM_PIN2, HIGH);
  int y = pulseIn(PWM_PIN4, HIGH);

  data.throttle = minmax(map(t, 1140, 1820, 0, 255), 0, 255);
  data.yaw = minmax(map(y, 1070, 1920, 0, 255), 0, 255);
  data.pitch = minmax(map(p, 1130, 1820, 0, 255), 0, 255);
  data.roll = minmax(map(r, 1120, 1850, 0, 255), 0, 255);
//  Serial.println(t);
//  Serial.println(r);
//  Serial.println(p);
//  Serial.println(y);
//  Serial.println("--------------");
}

/**************************************************/

void loop()
{
  lastRecvTime = millis();

  recvData();

  unsigned long now = millis();
  if ( now - lastRecvTime > 1000 ) {
    resetData();
  }

  setPPMValuesFromData();
}

/**************************************************/

//#error This line is here to intentionally cause a compile error. Please make sure you set clockMultiplier below as appropriate, then delete this line.
#define clockMultiplier 2 // set this to 2 if you are using a 16MHz arduino, leave as 1 for an 8MHz arduino

ISR(TIMER1_COMPA_vect) {
  static boolean state = true;

  TCNT1 = 0;

  if ( state ) {
    //end pulse
    PORTD = PORTD & ~B00000100; // turn pin 2 off. Could also use: digitalWrite(sigPin,0)
    OCR1A = PPM_PulseLen * clockMultiplier;
    state = false;
  }
  else {
    //start pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;

    PORTD = PORTD | B00000100; // turn pin 2 on. Could also use: digitalWrite(sigPin,1)
    state = true;

    if (cur_chan_numb >= channel_number) {
      cur_chan_numb = 0;
      calc_rest += PPM_PulseLen;
      OCR1A = (PPM_FrLen - calc_rest) * clockMultiplier;
      calc_rest = 0;
    }
    else {
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * clockMultiplier;
      calc_rest += ppm[cur_chan_numb];
      cur_chan_numb++;
    }
  }
}