
/******************************************************************************/
/** \file Eeprom.h
 **
 ** Add description here...
 **
 ** History:
 **   
 *****************************************************************************/

#ifndef EEPROM_H
#define EEPROM_H

#define	REAL_PARAMETER_START_ADDRESS  0x40		//ʵ�������ַ�У��洢��������ʼ��ַ

#define EEPROM_WORD_NUM         18             //�������������������±�



typedef union
{
	INT16U DataArray[EEPROM_WORD_NUM];
	INT8U  Data8UArray[(EEPROM_WORD_NUM) << 1];
	struct 
	{
	    INT16U MotorInputMode;
		INT16U MotorRunningMode;                //��RAM�д洢��ʽΪ00=���ֽڣ�01Ϊ���ֽ�

        INT16U HighDryPowerRom;
        INT16U HighWetPowerRom;
        INT16U HighDrySpeedRom;
        INT16U HighWetSpeedRom;
        
        INT16U MedDryPowerRom;
        INT16U MedWetPowerRom;
        INT16U MedDrySpeedRom;
        INT16U MedWetSpeedRom;
        
        INT16U LowDryPowerRom;
        INT16U LowWetPowerRom;        
        INT16U LowDrySpeedRom;
        INT16U LowWetSpeedRom;
        
        INT16U HighSpeed;
        INT16U MedSpeed;
        INT16U LowSpeed;
        
		INT16U  CheckSum;                
	}Data;
} EEpromStrUnion;

extern EEpromStrUnion    EepromParameter;
extern INT8U   	      g_WaitTime;
extern INT16U            g_ReadDataNum;
extern INT8U             g_CrcCheck;   


/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source)             */
/*****************************************************************************/

extern void InitIICPort(void);

extern void I2cWriteNByte(INT8U WriteStartAddress, INT8U *WriteData, INT8U WriteByteNumber);

extern void I2cReadNByte(INT8U ReadStartAddress,INT8U *ReadData,INT8U ReadByteNumber);

extern void UpdateEeprom(INT8U EepromWordOffset, INT16U Data);

extern void ReadEepromData(void);

#endif 

