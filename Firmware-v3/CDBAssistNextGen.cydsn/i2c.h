#ifndef I2C_H_
#define I2C_H_
    
#define I2CADDR_AD5241 (0x2c)
#define I2CADDR_LM75 (0x4f)
#define I2CADDR_ISL28023_VBAT (0x50)
#define I2CADDR_ISL28023_VBUS (0x54)

#define ISL28023_IOUT_CAL_GAIN (0x38)
#define ISL28023_CONFIG_ICHANNEL (0xd4)
#define ISL28023_CONFIG_VCHANNEL (0xd5)
#define ISL28023_READ_VSHUNT_OUT (0xd6)
#define ISL28023_READ_VOUT (0x8b)
#define ISL28023_READ_IOUT (0x8c)
#define ISL28023_READ_POUT (0x96)

void I2C_Init( void );
float I2C_Get_VBUS_Volt( void );
float I2C_Get_VBUS_Cur( void );
float I2C_Get_VBAT_Volt( void );
float I2C_Get_VBAT_Cur( void );
float I2C_Get_Temperature( void );
void I2C_Work( void );

#endif // I2C_H_
