#ifndef PWM_H_
#define PWM_H_

void PWM_Recalc(void);
void PWM_Init( void );
uint16_t PWM_GetVoltage(void);
float PWM_GetCurrent(void);
void PWM_Setpoints(uint16_t volt,uint16_t cur);
void PWM_Override( int16_t rawpwm );
int16_t PWM_GetOverride( void );
void PWM_Work(uint16_t actualvolt, int16_t actualcurraw);

#endif // PWM_H_
