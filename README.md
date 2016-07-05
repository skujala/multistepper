# multistepper
Stepper motor driver for Arduino using [Adafruit Motor Shield V2](https://learn.adafruit.com/adafruit-motor-shield-v2-for-arduino)

My attempt to simplify the seemingly convoluted and largely undocumented code Adafruit provides for their motor shield V2. Inspiration obtained from their code and chip datasheets. Verbatim copying tried to be kept at minimum. Work in progress. Designed to support stepper motors only! Ultimate goal is to be able to use it with STM32 Nucleo development boards. 

Missing elements:

* Computing how to move two motors "simultaneously"
* Acceleration and deceleration of the stepper motor
* ...

See datasheet [PCA9685](https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf) for more info. 

Public domain.