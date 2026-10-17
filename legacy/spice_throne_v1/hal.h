/* Hardware abstraction: time, buttons, LEDs, EEPROM, entropy.
 *
 * Wiring (unchanged from the original Meteor Dodge breadboard):
 *   LEFT  button  D2  (PD2)   active low, internal pull-up
 *   POWER button  D12 (PB4)   active low, internal pull-up
 *   RIGHT button  A0  (PC0)   active low, internal pull-up
 *   yellow LED    D4  (PD4)   soft PWM from Timer2 interrupts
 *   green LED 0   D3  (PD3)   OC2B hardware PWM
 *   green LED 1   D5  (PD5)   OC0B hardware PWM
 *   green LED 2   D6  (PD6)   OC0A hardware PWM
 */
#ifndef HAL_H
#define HAL_H

#include "platform.h"

#define BTN_LEFT  1
#define BTN_POWER 2
#define BTN_RIGHT 4

#define LED_YELLOW 3

void     hal_init(void);
uint16_t hal_ms(void);                 /* wrapping millisecond clock      */
void     hal_idle(void);               /* sleep until the next tick       */
void     hal_delay(uint16_t ms);
uint8_t  hal_btn_held(void);           /* debounced level, BTN_* mask     */
uint8_t  hal_btn_take(void);           /* press edges since last call     */
void     hal_pwm(uint8_t led, uint8_t level);
uint8_t  hal_ee_read(uint16_t addr);
void     hal_ee_write(uint16_t addr, uint8_t v);
uint32_t hal_entropy(void);

/* LED animation engine (leds.c), stepped every 16 ms from the tick. */
enum { LM_OFF, LM_ON, LM_BREATHE, LM_PULSE, LM_BLINK, LM_FADEOUT, LM_FADEIN, LM_FLICKER };
void led_set(uint8_t led, uint8_t mode, uint8_t level);
void leds_tick(void);

#endif
