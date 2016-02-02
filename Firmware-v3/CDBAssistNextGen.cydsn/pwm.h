#ifndef PWM_H_
#define PWM_H_

void PWM_Init( void );
uint16_t PWM_GetVoltage(void);
float PWM_GetCurrent(void);
void PWM_Setpoints(uint16_t volt,uint16_t cur, uint8_t range);
void PWM_Override( int16_t rawpwm );
int16_t PWM_GetOverride( void );
void PWM_Work(float actualvolt, float actualcur);

#endif // PWM_H_
