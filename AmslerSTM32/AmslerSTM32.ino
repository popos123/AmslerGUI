#include <Arduino.h>
#include <EncoderStepCounter.h>
#include <Adafruit_ADS1015.h>
#include <Chrono.h>

#define ADS1015_REG_CONFIG_DR_3300SPS (0x00C0)
#define ENCODER_PIN1 7 // Enc. Pin A
#define ENCODER_PIN2 16 // Enc. Pin B
#define ENCODER_INT1 digitalPinToInterrupt(ENCODER_PIN1)
#define ENCODER_INT2 digitalPinToInterrupt(ENCODER_PIN2)
byte gain = 0; // gain selection
word mean_number = 10; // number of average (10 = 10ms conv.)
uint32_t adc0s = 0, sum = 0, mean = 0, cnt = 0; // for mean adc
signed long position = 0; // encoder position

// communication
const byte buf_size = 20;
char LastCommand[buf_size];
word  serInIndx = 0;
char start_ = '(';
char end_ = ')';

// Use the following for half step encoders
EncoderStepCounter encoder(ENCODER_PIN1, ENCODER_PIN2, HALF_STEP);
// half 17590 steps, 100steps/1mm, L = 175,9mm, resolution = 10um
Adafruit_ADS1115 ads; // Use this for the 16-bit version
Chrono mean_t;

void readSerialString();
void writeSerialData();
void readData();
void interrupt();

void setup() {
  Serial.begin(500000);
  encoder.begin(); // Initialize encoder
  // Initialize interrupts
  attachInterrupt(ENCODER_INT1, interrupt, CHANGE);
  attachInterrupt(ENCODER_INT2, interrupt, CHANGE);
  ads.setGain(GAIN_ONE); // 1x gain +/- 4.096V
  ads.begin();
}

void loop() {
  readSerialString();
  writeSerialData();
  readData();
}

void readData() {
  // read from encoder (digital)
  signed char pos = encoder.getPosition();
  if (pos != 0) {
    position += pos;
    encoder.reset();
  }
  // read from ADC (analog)
  if (mean_t.hasPassed(1)) {
    mean_t.restart();
    uint16_t adc0 = ads.readADC_SingleEnded(0);
    sum += adc0;
    cnt++;
    if (cnt >= mean_number) adc0s = sum / cnt, cnt = 0, sum = 0;
  }
}

// Call tick on every change interrupt
void interrupt() {
  encoder.tick();
}

// Received data
void readSerialString() {
  char sb, read_data_in, serInString[buf_size];
  while (Serial.available()) {
    sb = Serial.read();
    if (sb == start_) read_data_in = 1; // start byte
    if (sb == end_) read_data_in = 0, serInIndx = 0; // end byte
    if (read_data_in == 1 && sb != start_ && sb != end_) {
      serInString[serInIndx] = sb;
      serInIndx++;
    }
  }
  if (sb == end_) {
    sprintf (LastCommand, "%s", serInString);
  }
}

// Send data
void writeSerialData() {
  if (strcmp(LastCommand, "EE") == 0) {
    Serial.println("Digital read set to 0");
    position = 0;
    encoder.reset();
    LastCommand[0] = '0';
  }
  if (strcmp(LastCommand, "RD") == 0) {
    Serial.print(String(adc0s) + " ");
    Serial.println(position);
    LastCommand[0] = '0';
  }
  if (strcmp(LastCommand, "GAIN1") == 0) {
    ads.setGain(GAIN_ONE); // 10mm 0-25k 0,005mm (5um)
    ads.begin();
  }
  if (strcmp(LastCommand, "GAIN2") == 0) {
    ads.setGain(GAIN_TWO); // 6,5mm 0-32k 0,002mm (2um)
    ads.begin();
  }
  if (strcmp(LastCommand, "GAIN3") == 0) {
    ads.setGain(GAIN_FOUR); // 3,2mm 0-32k 0,001mm (1um)
    ads.begin();
  }
  if (strcmp(LastCommand, "GAIN4") == 0) {
    ads.setGain(GAIN_EIGHT); // 1,58mm 0-32k 0,0005mm (0.5um)
    ads.begin();
  }
  if (LastCommand[0] == 'G' && LastCommand[1] == 'A') {
    Serial.println("Gain set to " + String(LastCommand[4]));
    LastCommand[0] = '0';
  }
}
