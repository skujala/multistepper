#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>


#define PCA9685_MODE1 0x0
#define PCA9685_PRESCALE 0xFE

#define LED0_ON_L 0x6

#define DIRECTION_FORWARD   1
#define DIRECTION_BACKWARD -1


typedef struct {
  uint8_t pwm_a_pin, a_in1_pin, a_in2_pin;
  uint8_t pwm_b_pin, b_in1_pin, b_in2_pin;
  
  uint8_t *latch_state;
  
  uint8_t i2c_addr;
} stepper_state_t;


stepper_state_t stepper1;
stepper_state_t stepper2;

uint8_t latch_states[] = {0x08, 0x0A, 0x02, 0x06, 0x04, 0x05, 0x01, 0x09};

void setup()
{
  init_pca9685(0x60, 1600);
  init_stepper(0, 0x60, &stepper1);
  init_stepper(1, 0x60, &stepper2);
}


void loop()
{
  set_pin(&stepper1, stepper1.pwm_a_pin, 1);
  set_pin(&stepper1, stepper1.pwm_b_pin, 1);
  set_pin(&stepper2, stepper2.pwm_a_pin, 1);
  set_pin(&stepper2, stepper2.pwm_b_pin, 1);
  
  for (uint16_t i = 0; i < 2000; i++)
  {
    one_step(&stepper2, DIRECTION_BACKWARD);
    one_step(&stepper1, DIRECTION_FORWARD);
    delayMicroseconds(10);
  }
    
  delay(250);
  
  for (uint16_t i = 0; i < 2000; i++)
  {
    one_step(&stepper2, DIRECTION_FORWARD);    
    one_step(&stepper1, DIRECTION_BACKWARD);
    delayMicroseconds(50);
  }

  set_pin(&stepper1, stepper1.pwm_a_pin, 0);
  set_pin(&stepper1, stepper1.pwm_b_pin, 0);
  set_pin(&stepper2, stepper2.pwm_a_pin, 0);
  set_pin(&stepper2, stepper2.pwm_b_pin, 0);
  
  delay(25000);  
}


void init_stepper(uint8_t num, uint8_t i2c_addr, stepper_state_t *stepper)
{
  stepper->i2c_addr = i2c_addr;
  stepper->latch_state = &latch_states[0];
  
  /* Initialize which PCA9685 pins belong to which motor */
  
  switch(num) {
    case 0: /* Labeled M1 and M2 */
      stepper->pwm_a_pin = 8;
      stepper->a_in2_pin = 9;
      stepper->a_in1_pin = 10;
      stepper->b_in1_pin = 11;
      stepper->b_in2_pin = 12;
      stepper->pwm_b_pin = 13;
      break;
    
    case 1: /* Labeled M1 and M2 */
      stepper->pwm_a_pin = 2;
      stepper->a_in2_pin = 3;
      stepper->a_in1_pin = 4;
      stepper->b_in1_pin = 5;
      stepper->b_in2_pin = 6;
      stepper->pwm_b_pin = 7;
      break;
  }  
}


void one_step(stepper_state_t *stepper, int8_t direction)
{
  uint8_t msg[4];
  
  if (direction == DIRECTION_BACKWARD && stepper->latch_state == &latch_states[0]) {
    stepper->latch_state = &latch_states[7];
  } else if (direction == DIRECTION_FORWARD && stepper->latch_state == &latch_states[7]) {
    stepper->latch_state = &latch_states[0];
  } else {
    stepper->latch_state += direction; 
  }
  
  msg[0] = *stepper->latch_state & 0x4; // A_in2
  msg[1] = *stepper->latch_state & 0x8; // A_in1
  msg[2] = *stepper->latch_state & 0x2; // B_in1
  msg[3] = *stepper->latch_state & 0x1; // B_in2
   
  set_pins(stepper, msg);
}


static void set_pin(stepper_state_t *stepper, uint8_t pin, uint8_t value)
{
  set_pin_pwm_dutycycle(stepper->i2c_addr, pin, (value == 0 ? 0 : 4096), 0);
}

static void set_pins(stepper_state_t *stepper, uint8_t *values)
{
  uint8_t on, i;
  
  Wire.beginTransmission(stepper->i2c_addr);

  /* 
    Correct order in the message
      1. A_in2_pin
      2. A_in1_pin
      3. B_in1_pin
      4. B_in2_pin
   */
  Wire.write(LED0_ON_L + 4 * stepper->a_in2_pin);

  for (i = 0; i < 4; i++) {
    on = (*values == 0 ? 0 : 0x10);
    Wire.write(0x00); // ON value low byte
    Wire.write(on);   // ON value high byte 
    Wire.write(0x00); // OFF value low byte
    Wire.write(0x00); // ON value high byte
    
    values++;
  }
  
  Wire.endTransmission();
}

void init_pca9685(uint8_t i2c_addr, uint16_t freq_Hz)
{
  Wire.begin();
 
  /* Hack to increase I2C speed for faster comms 
     NOTE: for my motors 800000L is too fast if only motor is used
   */
  #define TWI_FREQ 400000L
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
  
  // reset PCA9685
  write_pca9885_byte(i2c_addr, PCA9685_MODE1, 0x0);
  
  set_pwm_frequency(i2c_addr, freq_Hz);
  
  for (uint8_t pin = 0; pin < 16; pin++) {
    set_pin_pwm_dutycycle(i2c_addr, pin, 0, 0);
  }
}



static void set_pwm_frequency(uint8_t i2c_addr, uint16_t freq_Hz) 
{
  uint16_t oscillator = (25000000 << 12);
  uint8_t prescaler = oscillator / freq_Hz;
  
  uint8_t oldmode = read_pca9885_byte(i2c_addr, PCA9685_MODE1);
  
  write_pca9885_byte(i2c_addr, PCA9685_MODE1, (oldmode & 0x7F) | 0x10); // go to sleep
  write_pca9885_byte(i2c_addr, PCA9685_PRESCALE, prescaler);            // set the prescaler
  write_pca9885_byte(i2c_addr, PCA9685_MODE1, oldmode);                 // restore operation
  delay(5);                                       // Wait 
  
  // Set MODE1 register to autoincrement
  // Besides, why is this 0xA1 -- Restart, Auto increment, and Respond to All Call I2C
  write_pca9885_byte(i2c_addr, PCA9685_MODE1, oldmode | 0xa1);  
}

void set_pin_pwm_dutycycle(uint8_t i2c_addr, uint8_t num, uint16_t on, uint16_t off) {
  Wire.beginTransmission(i2c_addr);

  Wire.write(LED0_ON_L + 4 * num);
  Wire.write((on & 0xFF));       // ON value low byte
  Wire.write((on >> 8) & 0xFF); // ON value high byte 
  Wire.write(off & 0xFF);        // OFF value low byte
  Wire.write((off >> 8) & 0xFF); // ON value high byte

  Wire.endTransmission();
}


static uint8_t read_pca9885_byte(uint8_t i2c_addr, uint8_t addr) {
  Wire.beginTransmission(i2c_addr);
  Wire.write(addr);
  Wire.endTransmission(); /* partial transfer */

  Wire.requestFrom((uint8_t)i2c_addr, (uint8_t)1);
  return Wire.read();
}

static void write_pca9885_byte(uint8_t i2c_addr, uint8_t addr, uint8_t d) {
  Wire.beginTransmission(i2c_addr);
  Wire.write(addr);
  Wire.write(d);
  Wire.endTransmission();
}

