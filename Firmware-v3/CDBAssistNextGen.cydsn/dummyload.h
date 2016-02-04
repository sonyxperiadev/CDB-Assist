#ifndef DUMMYLOAD_H_
#define DUMMYLOAD_H_
    
float DummyLoad_DAC_GetGain(int8_t gainadj);
float DummyLoad_ADC_GetGain(int8_t gainadj);
void DummyLoad_CalcGain(uint8_t zerodac);
void DummyLoad_Init( void );
void DummyLoad_Setpoints(uint16_t minvolt, uint16_t maxcur);
void DummyLoad_Work(uint16_t actualvolt);
void DummyLoad_ADCWork(void);
float DummyLoad_GetCur(void);
float DummyLoad_GetTemp(void);

#endif // DUMMYLOAD_H_
