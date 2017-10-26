/******************************************************************************/
/** \file UART.c
 **
 ** Add description here...
 **
 ** History:
 **   
 *****************************************************************************/
#include "InitMcu.h"
#include "system_s6e1a1.h"
#include "s6e1a1.h"
#include "base_types.h"
#include "Isr.h"
#include "Eeprom.h"

#define	 I2C_SDA_INPUT()    (FM0P_GPIO->PDIR4_f.P46)

#define  SetSdaOutput()     (FM0P_GPIO->DDR4_f.P46 = 1)
#define  SetSdaInput()      (FM0P_GPIO->DDR4_f.P46 = 0)

#define  SetSDAHigh()       (FM0P_GPIO->PDOR4_f.P46 = 1)
#define  SetSDALow()        (FM0P_GPIO->PDOR4_f.P46 = 0)
#define  SetSCLKHigh()      (FM0P_GPIO->PDOR4_f.P47 = 1)
#define  SetSCLKLow()       (FM0P_GPIO->PDOR4_f.P47 = 0)
#define	 I2C_DELAY()		{asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP");asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP"); asm("\tNOP");}

#define	 AT24C02_WR_COMMAND		0xa0		//EEPROM������ַ+д����
#define	 AT24C02_RD_COMMAND		0Xa1		//������

#define  TIME_20MS       5

EEpromStrUnion    EepromParameter;
INT16U            g_ReadDataNum;
INT8U             g_ReadDataTimes;
INT8U             g_CrcCheck;                           //  1crcУ�飬0�ۼӺ�У��
INT8U   	      g_WaitTime;


/**************************************************************************
*    ����ԭ��:void InitIICPort(void)
*    ��    ��:
**************************************************************************/
void InitIICPort(void)
{
    FM0P_GPIO->SPSR_f.SUBXC0 = 0;  //X0A,X1A USED AS IO
    FM0P_GPIO->SPSR_f.SUBXC1 = 0;

    //IIC-SDA
    FM0P_GPIO->PFR4_f.P46 = 0;              // disable peripheral function 
    FM0P_GPIO->PCR4_f.P46 = 1; 
    FM0P_GPIO->DDR4_f.P46 = 1;                // output pin
    FM0P_GPIO->PDOR4_f.P46 = 1;	
  
    //IIC-SCLK
    FM0P_GPIO->PFR4_f.P47 = 0;              // disable peripheral function 
    FM0P_GPIO->PCR4_f.P47 = 1; 
    FM0P_GPIO->DDR4_f.P47 = 1;                // output pin
    FM0P_GPIO->PDOR4_f.P47 = 1;	     
}


/**************************************************************************
*    ����ԭ��: void Delayms(unsigned char Time)
*    ��    ��:��ʱ,rWaitTimeһ����ֵΪ2ms
**************************************************************************/
void Delayms(INT8U Time)
{
	g_WaitTime = 0; 
	
	while(g_WaitTime < Time)
	{
        Hwwdg_Feed(0x55,0xAA);
        Swwdg_Feed();        
    }
}

/**************************************************************************
*    ����ԭ��:void I2cStart()
*    ��    ��:I2C��ʼ����
**************************************************************************/
void I2cStart()
{
	SetSdaOutput();
	I2C_DELAY();	
	SetSDAHigh();	
	I2C_DELAY();
	SetSCLKHigh();
	I2C_DELAY();
	SetSDALow();
	I2C_DELAY();
	SetSCLKLow();	
	I2C_DELAY();
}
/**************************************************************************
*    ����ԭ��:void I2cStop()
*    ��    ��:I2C��ʼ����
**************************************************************************/
void I2cStop()
{
	SetSdaOutput();
	I2C_DELAY();	
	SetSDALow();
	I2C_DELAY();
	SetSCLKHigh();
	I2C_DELAY();
	SetSDAHigh();
	I2C_DELAY();
}
/**************************************************************************
*    ����ԭ��:void SendAck()
*    ��    ��:����ack��Ӧ
**************************************************************************/
void SendAck()
{
	SetSdaOutput();;
	I2C_DELAY();
	SetSDALow();
	I2C_DELAY();
	SetSCLKHigh();
	I2C_DELAY();
	SetSCLKLow();
	I2C_DELAY();
}
/**************************************************************************
*    ����ԭ��:void SendNoAck()
*    ��    ��:����noack��Ӧ
**************************************************************************/
void SendNoAck()
{
	SetSdaOutput();
	I2C_DELAY();
	SetSDAHigh();
	I2C_DELAY();
	SetSCLKHigh();
	I2C_DELAY();
	SetSCLKLow();
	I2C_DELAY();
}
/**************************************************************************
*    ����ԭ��:void CheckAck()
*    ��    ��:�ȴ���Ӧ��Ӧ
**************************************************************************/
void CheckAck()
{
	INT8U i = 0;
	
	SetSCLKLow();
	SetSDAHigh();
	SetSdaInput();
	I2C_DELAY();
	
	SetSCLKHigh();
	while((I2C_SDA_INPUT()) && (i < 20))			//������ȡ
	{
		i++;
	}
	SetSCLKLow();
	I2C_DELAY();	
}
/**************************************************************************
*    ����ԭ��:void I2cWrite(INT8U WriteData)
*    ��    ��:д����
*    ��    ��:WriteData �����͵�����
*    ��    ��:
*    ȫ�ֱ���:
*    ����ģ��:
*    ע������:
**************************************************************************/
void I2cWrite(INT8U WriteByte)
{
	INT8U i = 0;	 
	
	SetSdaOutput();
	I2C_DELAY();
	for(i = 0;i < 8;i++)
	{
		SetSCLKLow();
		if(WriteByte & 0x80)
        {
			SetSDAHigh();
        }
		else 
        {
			SetSDALow();	
        }
					
		WriteByte <<= 1;
		SetSCLKHigh();
		I2C_DELAY();
		SetSCLKLow();
		I2C_DELAY();		
	}	
}
/**************************************************************************
*    ����ԭ��:INT8U I2cRead(void)
*    ��    ��:������
*    ��    ��:
*    ��    ��:return ��ȡ��������ReadByte
*    ȫ�ֱ���:
*    ����ģ��:
*    ע������:
**************************************************************************/
INT8U I2cRead(void)
{
	INT8U i = 0;
	INT8U ReadByte = 0;
	
	SetSCLKLow();	
	SetSdaInput();	
	I2C_DELAY();
	for(i = 0;i < 8;i++)
	{
		I2C_DELAY();
		
		SetSCLKHigh();
		I2C_DELAY();
				
		ReadByte <<= 1;
		
		if (I2C_SDA_INPUT())
        {
            ReadByte |= 0x01;
        }		
		
		SetSCLKLow();		
	}	
	return(ReadByte);
}
/**************************************************************************
*    ����ԭ��:void I2cWriteNByte(INT8U WriteStartAddress,INT8U *WriteData,INT8U WriteByteNumber)
*    ��    ��:д�����ֽ�
*    ��    ��:Ҫ����24c02����ʼ��ַReadStartAddress����д���ݵ�ָ��WriteData����Ҫд�ĸ���WriteByteNumber
*    ��    ��:
*    ȫ�ֱ���:
*    ����ģ��:I2cWrite();CheckAck();
*    ע������:����д���ֻ��д8��,//cllע:���ܿ�Խ8�ֽڵı߽�,��8,16,24
**************************************************************************/
void I2cWriteNByte(INT8U WriteStartAddress, INT8U *WriteData, INT8U WriteByteNumber)
{
	INT8U i = 0;	
	
	I2cStart();	
	I2cWrite(AT24C02_WR_COMMAND);			//��д����	
	CheckAck();
	I2cWrite(WriteStartAddress);
	CheckAck();
	
	Delayms(TIME_20MS);	
	
	for(i = 0;i < WriteByteNumber;i++)
	{
        Hwwdg_Feed(0x55,0xAA);
        Swwdg_Feed();           
	       
		I2cWrite(*WriteData);
		CheckAck();
		WriteData++;
	}
	I2cStop();
	Delayms(TIME_20MS);					//�ȴ�ʱ����Ҫ��һЩ
}
/**************************************************************************
*    ����ԭ��:void I2cReadNByte(INT8U ReadStartAddress,INT8U *ReadData,INT8U ReadByteNumber)
*    ��    ��:д�����ֽ�
*    ��    ��:Ҫ����24c02����ʼ��ַReadStartAddress���������ݱ����ָ��ReadData����Ҫд�ĸ���ReadByteNumber
*    ��    ��:
*    ȫ�ֱ���:
*    ����ģ��:I2cWrite();I2cWrite();CheckAck();
*    ע������:
**************************************************************************/
void I2cReadNByte(INT8U ReadStartAddress,INT8U *ReadData,INT8U ReadByteNumber)
{
	INT8U i = 0;
	
	I2cStart();
	
	I2cWrite(AT24C02_WR_COMMAND);
	CheckAck();
	I2cWrite(ReadStartAddress);
	CheckAck();
	
	I2C_DELAY();
    Hwwdg_Feed(0x55,0xAA);
    Swwdg_Feed(); 
   
	I2cStart();
		
	I2cWrite(AT24C02_RD_COMMAND);
	CheckAck();
	
	Delayms(TIME_20MS);
	
	for(i = 0;i < ReadByteNumber;i++)
	{		
		*ReadData = I2cRead();
		if(i == (ReadByteNumber - 1))
		{
			SendNoAck();		//���һλ��NOACK
		}
		else
		{
			SendAck();
	    }
		ReadData++;
		I2C_DELAY();			//�˴���Ҫ��΢�ӳ�һ�£��������
	}		
	I2C_DELAY();				//�˴���ҲҪ��΢�ӳ�һ��
	I2cStop();
	Delayms(TIME_20MS);
}
/**************************************************************************
*    ����ԭ��: INT16U GetAccumulateSum(INT8U *ptr,INT8U len)
*    ��    ��:  �ۼӺ�У��
**************************************************************************/
INT16U GetAccumulateSum(INT8U *ptr,INT8U len)
{
    INT8U i = 0;
    
    INT16U  AccSum = 0;
    
    for (i = 0; i < len; i++)
    {
        AccSum += *ptr;
        ptr++;
    }
    
    return(AccSum);
}

/**************************************************************************
*    ����ԭ��: void SaveDataToEEprom()
*    ��    ��: ��������
*    ˵    ��: 
*    ��    ��: 
*    ��    ��: 
**************************************************************************/
void SaveDefaultToEEprom(void)
{
	INT16U CheckSum = 0;
	INT8U  i = 0;
	INT8U  tmp = 0;
	
	INT16U Address = 0;
	INT8U  Data[2] = {0,0};

    EepromParameter.Data.MotorInputMode = THREE_LEVEL_MODE;
	EepromParameter.Data.MotorRunningMode = POWER_MODE;
	
	//EepromParameter.Data.HighPower = 0x3200;  //50w
	//EepromParameter.Data.MedPower  = 0x1E00;  //30w
	//EepromParameter.Data.LowPower  = 0x0A00;  //10w
	EepromParameter.Data.HighDryPowerRom = 0x3200;  //50w
	EepromParameter.Data.MedDryPowerRom  = 0x1E00;  //30w
	EepromParameter.Data.LowDryPowerRom  = 0x0A00;  //10w
	
	EepromParameter.Data.HighWetPowerRom = 0x3200;  //50w
	EepromParameter.Data.MedWetPowerRom  = 0x1E00;  //30w
	EepromParameter.Data.LowWetPowerRom  = 0x0A00;  //10w

	EepromParameter.Data.HighDrySpeedRom = 0x03E8;   //1000RPM
	EepromParameter.Data.MedDrySpeedRom  = 0x0320;  //800RPM
	EepromParameter.Data.LowDrySpeedRom  = 0x01F4;  //500RPM
	
	EepromParameter.Data.HighWetSpeedRom = 0x03E8;  //50w
	EepromParameter.Data.MedWetSpeedRom  = 0x0320;  //30w
	EepromParameter.Data.LowWetSpeedRom  = 0x01F4;  //10w
	
	EepromParameter.Data.HighSpeed = 1200;
	EepromParameter.Data.MedSpeed  = 800;
	EepromParameter.Data.LowSpeed  = 500;
	
	if (g_CrcCheck)
	{
	    CheckSum = CRC16(&EepromParameter.Data8UArray[0],((g_ReadDataNum - 1) << 1));	    
	}
	else
	{
	    CheckSum = GetAccumulateSum(&EepromParameter.Data8UArray[0],((g_ReadDataNum - 1)<< 1));
	}
	 
	EepromParameter.Data.CheckSum = CheckSum;             //ע�⣬�ߵ��ֽ�˳��ͬ������ֱ�Ӹ�ֵ
	
	for (i = 0; i <= g_ReadDataNum; i++)
	{
		tmp = (i << 1);
		Address = REAL_PARAMETER_START_ADDRESS + tmp;
		
		Data[0] = EepromParameter.Data8UArray[tmp];
		Data[1] = EepromParameter.Data8UArray[tmp + 1];
		I2cWriteNByte(Address,&Data[0],2);                  //ÿ��д2byte
        Hwwdg_Feed(0x55,0xAA);
        Swwdg_Feed();      
	}
}

/**************************************************************************
*    ����ԭ��: void ReadEepromData()
*    ��    ��: ��ȡ����
*    ˵    ��: 
*    ��    ��: 
*    ��    ��: 
**************************************************************************/
void ReadEepromData(void)
{
    INT16U CheckSum = 0;
    INT16U CheckSum1 = 0;
    INT16U Address = 0;
    INT8U  i   = 0;
    INT8U  Tmp = 0;  
    INT8U  Data[2] = {0,0};
       
    g_ReadDataNum = EEPROM_WORD_NUM ;
    while(g_ReadDataTimes < 3)                                      //����3��
    {
        for (i = 0; i <= g_ReadDataNum; i++)
        {
            Address = REAL_PARAMETER_START_ADDRESS + (i << 1);
            I2cReadNByte(Address,&Data[0],2);		             //ÿ�ζ�ȡ2�ֽ�
            Hwwdg_Feed(0x55,0xAA);
            Swwdg_Feed(); 
                	
            EepromParameter.DataArray[i] = (Data[1] << 8) | Data[0];            
        }       
        	
        //--------�ж��ۼ�У���
        if (!g_CrcCheck)
        {            
            if (EepromParameter.Data.CheckSum == GetAccumulateSum(&EepromParameter.Data8UArray[0],((g_ReadDataNum - 1)<< 1)))
            {
                CheckSum1 = 0;
            }
            else
            {
                CheckSum1 = 0xaa;
            }
            
            if (CheckSum1 == 0)
            {
                g_ReadDataTimes = 0;
                break;
            }
            else
            {
                g_ReadDataTimes++;
            }    
        }
        else
        {                    
            //---����CRCУ���֮ǰ����Ҫ��EepromParameter.Data.CheckSum�ĸߵ�λ�Ե�  
            Tmp = (g_ReadDataNum - 1) << 1;
            i = EepromParameter.Data8UArray[Tmp];                       //i����ʱ������
            EepromParameter.Data8UArray[Tmp] = EepromParameter.Data8UArray[Tmp + 1];
            EepromParameter.Data8UArray[Tmp +1] = i;        
            //---�ж�CRCУ���
            CheckSum = CRC16(&EepromParameter.Data8UArray[0],(g_ReadDataNum << 1));
		
            if (CheckSum == 0)
            {
                g_ReadDataTimes = 0;
                break;
            }
            else
            {
                g_ReadDataTimes++;
            }    
        }	    
    }          
        
    if(g_ReadDataTimes != 0)
    {
        g_ReadDataTimes = 0;        
        //----------set default		 
        SaveDefaultToEEprom();
    }     
}

/**************************************************************************
*    ����ԭ��: void UpdateEeprom(INT8U EepromOffset, INT16U Data)
*    ��    ��: 
**************************************************************************/
void UpdateEeprom(INT8U EepromWordOffset, INT16U Data)
{
    INT8U EepromAddress = 0;
    INT8U aDataBuff[2] = {0,0};
    
    aDataBuff[0] = Data;		//ȡ��λ
    aDataBuff[1] = Data >> 8;	//ȡ��λ
    
    EepromAddress = (EepromWordOffset << 1) + REAL_PARAMETER_START_ADDRESS;        	//��ַ��Ҫ�Ŵ�һ��������0x40Ϊeeprom��ʼ��ַ
    
    I2cWriteNByte(EepromAddress,&aDataBuff[0],2);               //д2�ֽ�
}


