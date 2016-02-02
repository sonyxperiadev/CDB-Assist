#ifndef DUMMYLOAD_H_
#define DUMMYLOAD_H_
    
void DummyLoad_Init( void );
void DummyLoad_Setpoints(uint16_t minvolt, uint16_t maxcur);
void DummyLoad_Work(float actualvolt);
void DummyLoad_ADCWork(void);
float DummyLoad_GetCur(void);
float DummyLoad_GetTemp(void);

#endif // DUMMYLOAD_H_
