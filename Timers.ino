#include "Timers.h"

void IRAM_ATTR timer1_ISR()
{
  wave();
  TZXLoop();
}

void TimerClass::stop(void)
{
  timer1_disable();
}

void TimerClass::setPeriod(unsigned long period)
{
  //timer1_write(period * 80); //TIM_DIV1 gives 80 per 1us for ESP8266 (80 MHz I/O)
  //timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);
  
  timer1_write(period * 5); //TIM_DIV16 gives 5 per 1us for ESP8266 (80 MHz I/O)
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
}

void TimerClass::initialize()
{
  timer1_disable();
  timer1_attachInterrupt(timer1_ISR);
}

TimerClass Timer;
