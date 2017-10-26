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
#include "Uart.h"
#include "Eeprom.h"

SetCommandUnion   SetCommand;
RuningStatusUnion RunParameter;

/*************************************************/

stc_uart_rec st_UartRec;

stc_uart_tx st_UartTx;

//----Modbus�й�
#define MODBUS_ADDRESS                  0x55
#define MODBUS_READ                     0x03
#define MODBUS_WRITE_SINGLE             0x06
#define MODBUS_WRITE_MULTI              0x16
#define MODBUS_WRITE_AND_READ           0x60

#define MODBUS_READ_ACK                 0x03
#define MODBUS_WRITE_SINGLE_ACK         0x06
#define MODBUS_WRITE_MULTI_ACK          0x10
#define MODBUS_WRITE_AND_READ_ACK       0x60

#define READ_ORDER_START_ADDRESS        0x0000
#define READ_ORDER_END_ADDRESS          0x0003          //MODBUS ���������ʼ������ַ,��ͨѶЭ��Ĵ�����ַ����


#define READ_STATUS_START_ADDRESS       0x0100
#define READ_STATUS_END_ADDRESS         0x010E          //MODBUS ��״̬����ʼ������ַ

#define READ_EEPROM_START_ADDRESS       0x1020
#define READ_EEPROM_END_ADDRESS         (WRITE_EEPROM_START_ADDRESS + EEPROM_WORD_NUM -1)          //MODBUS ��eeprom����ʼ������ַ


#define WRITE_COMMAND_START_ADDRESS     0x0000
#define WRITE_COMMAND_END_ADDRESS       0x0005          //MODBUS д�������ʼ������ַ

#define WRITE_EEPROM_START_ADDRESS      0x1020
#define WRITE_EEPROM_END_ADDRESS        (WRITE_EEPROM_START_ADDRESS + EEPROM_WORD_NUM -1)//MODBUS дEEPROM����ʼ������ַ

#define	CHECKSUM_ADDRESS		WRITE_EEPROM_END_ADDRESS		//���һ����ַ��checksum,���϶����߼���ַ.�������ַ��ͬ
/**************************************************************************
*    ����ԭ��:void InitUart(void)
*    ��    ��:
**************************************************************************/
void InitUart3(void)
{
    FM0P_GPIO->PFR2_f.P22 = 1;                // act as peripheral function
    FM0P_GPIO->DDR2_f.P22 = 0;                    // input pin
    FM0P_GPIO->PCR2_f.P22 = 1;
    
    FM0P_GPIO->PFR2_f.P21 = 1;                // act as peripheral function
    FM0P_GPIO->DDR2_f.P21 = 1;   
    FM0P_GPIO->PCR2_f.P21 = 1;
    
    FM0P_GPIO->EPFR07_f.SIN0S = 0;
    FM0P_GPIO->EPFR07_f.SOT0B = 1;

    FM0P_MFS0_UART->SCR_f.UPCL = 1;       //UPCL=1 programmerble clear

    FM0P_MFS0_UART->BGR = 4165;   // Value=((bus clock/baudrate)-1) APB2=40MHz ,error>2%  bdr=9600

    FM0P_MFS0_UART->SMR = 0;   // bit7~5:MD2~0=0b000 mode0 asynchronize normal;LSB first;1 bit stop

    FM0P_MFS0_UART->SMR_f.SOE = 1; // bit0:SOE=1 serial data output enable  

    FM0P_MFS0_UART->ESCR = 0;  // bit7:FLWEN=0 hardwere flow disable  ;NRZ format; disable parity;8-bit Data length

    FM0P_MFS0_UART->SCR_f.RXE = 1;   //en rx
    FM0P_MFS0_UART->SCR_f.RIE = 1;   // en rx int 


    /*��֡��ʱ��*/
    FM0P_BT3_RT->TMCR = 0x2030;//CKS3-CKS0:1010 -> CLK/2048,trigger input disable
    FM0P_BT3_RT->TMCR2 = 0x01;

    FM0P_BT3_RT->STC = 0x10;        //Enable underflow interrupt,Clear interupt cause
    FM0P_BT3_RT->PCSR = 78;
    FM0P_BT3_RT->TMR = 78;
    //FM0P_BT3_RT->TMCR|= 0x03;      //software trigger and count start 
}
/**************************************************************************
*    ����ԭ��: INT16U CRC16(void)
*    ��    ��: ����ʽX16+X12+X5+X1
**************************************************************************/ 
INT16U CRC16(INT8U *ptr,INT8U len) // ptr Ϊ����ָ�룬len Ϊ���ݳ���
{
    INT8U i;
    INT16U crc = 0;
    while(len--)
    {
        for(i=0x80; i!=0; i>>=1)
        {
            if((crc&0x8000)!=0) {crc<<=1; crc^=0x1021;} 
            else crc<<=1;  
            if((*ptr&i)!=0) crc^=0x1021;  
        }
        ptr++;
    }
    return(crc);
}
/**************************************************************************
*    ����ԭ��: void ReadyUartTx(void)
*    ��    ��:
**************************************************************************/
void ReadyUartTx(void)
{ 
    st_UartTx.TxCount = 0;      

    FM0P_MFS0_UART->SCR_f.TIE = 1;
    FM0P_MFS0_UART->SCR_f.TXE = 1;
}

/**************************************************************************
*    ����ԭ��: INT16U CheckSumPrc()
*    ��    ��: 
**************************************************************************/
INT16U CheckSumPrc()
{
    INT16U CheckCode = 0;
    INT16U RecCheck = 0; 
    INT8U  i = 0;

    
    if ((st_UartRec.RxDataBuff[1]& 0x80) == 0)
    {       //--CRCУ��
            
        st_UartRec.CrcCheckFlag = 1;

        //Ptr = st_UartRec.RxDataBuff;
        CheckCode = CRC16(&st_UartRec.RxDataBuff[0],st_UartRec.MaxRxNumber);         
        
        if (CheckCode == 0x00)
        {
            return(1);              //У������ȷ
        }
        else
        {
            return(0);
        }
    }
    else
    {
        st_UartRec.CrcCheckFlag = 0;
        //--�ۼӺ�У��
        for (i = 0; i < (st_UartRec.MaxRxNumber - 2); i++)
        {
            CheckCode += st_UartRec.RxDataBuff[i];
        }

        RecCheck = (INT16U)((st_UartRec.RxDataBuff[st_UartRec.MaxRxNumber - 2] << 8)+ st_UartRec.RxDataBuff[st_UartRec.MaxRxNumber - 1]);

        if (CheckCode == RecCheck)
        {
            return(1);              //У������ȷ
        }
        else
        {
            return(0);
        }
    }
}
/**************************************************************************
*    ����ԭ��: INT16U GetRegisterData(INT16U Address)
*    ��    ��: ��ȡ��Ӧ��ַ�е�����
**************************************************************************/
INT16U GetRegisterData(INT16U Address)
{
    INT8U   Offset = 0;
    INT16U  GetData = 0;
    
    if ((Address >= READ_ORDER_START_ADDRESS) && (Address <= READ_ORDER_END_ADDRESS))
    {
        Offset = (Address - READ_ORDER_START_ADDRESS);  
        
        GetData = SetCommand.SetDataArray[Offset];
    }
    else if ((Address >= READ_STATUS_START_ADDRESS) && (Address <= READ_STATUS_END_ADDRESS))
    {
        Offset = (Address - READ_STATUS_START_ADDRESS);  
        
        GetData = RunParameter.ReadDataArray[Offset];                
            
    }
    else if ((Address >= READ_EEPROM_START_ADDRESS) && (Address <= READ_EEPROM_END_ADDRESS))
    {
        //EEPROM�еĲ�����RAM��������ͬ���ģ�ֻ��Ҫ�ṩRAM�����Ϳ���
        Offset = (Address - READ_EEPROM_START_ADDRESS);  
        
        GetData = EepromParameter.DataArray[Offset];      
    }
    else
    {
        GetData = 0;
    }
    
    return(GetData);
}
/**************************************************************************
*    ����ԭ��: void ReadyReadCmdAckData(void)
*    ��    ��: ���յ�read�����Ĵ���
**************************************************************************/
void ReadyReadCmdAckData(void)
{
    INT8U  i = 0;
    INT8U  ReadByteNum = 0;
    INT8U  ByteId = 0;
     
    INT16U StartAddress = 0;                                //��Ҫ��ȡ�Ĵ�������ʼ��ַ
    INT16U Address = 0;
    INT16U Data = 0;
     
    INT16U CheckSum = 0;

    ReadByteNum = st_UartRec.RxDataBuff[5];         //���͵�������Ŀ=�Ĵ�����    
    if (ReadByteNum > MAX_READ_REGISTER_NUM)
    {
        ReadByteNum = MAX_READ_REGISTER_NUM;           //�����Ŀ����
    }
       
    StartAddress = ((st_UartRec.RxDataBuff[2] << 8) | st_UartRec.RxDataBuff[3]);        //ȡ�׵�ַ

    st_UartTx.TxDataBuff[0] = MODBUS_ADDRESS;
    st_UartTx.TxDataBuff[1] = st_UartRec.RxDataBuff[1];                      
    st_UartTx.TxDataBuff[2] = ReadByteNum << 1;                          //���͵�byte��Ŀ

    Address = StartAddress;                 
    ByteId = 3; 
    for (i = 0; i < ReadByteNum; i++)
    {
        Data = GetRegisterData(Address);                
        st_UartTx.TxDataBuff[ByteId] = Data >> 8;                    //high byte
        ByteId++;                
        st_UartTx.TxDataBuff[ByteId] = Data;                         //low byte
        ByteId++;
        Address++;                            
    }
    
    //--���������ж�������У���
    if ((st_UartRec.RxDataBuff[1]& 0x80) == 0)   //CRCУ��
    {        
        CheckSum = CRC16(&st_UartTx.TxDataBuff[0],ByteId);
    }
    else
    {
        for (i = 0; i < ByteId; i++)
        {
            CheckSum += st_UartTx.TxDataBuff[i];         
        }
    }     

    st_UartTx.TxDataBuff[ByteId] = CheckSum >>8;                         //��λ
    ByteId++;
    st_UartTx.TxDataBuff[ByteId] = CheckSum ;                            //��λ
    ByteId++;  

    st_UartTx.MaxTxNumber = ByteId;   
}
/**************************************************************************
*    ����ԭ��: void ReadyWriteSingleAckData(void)
*    ��    ��: 
**************************************************************************/
void ReadyWriteSingleAckData()
{
    INT8U i = 0;
    
    for (i = 0; i < st_UartRec.MaxRxNumber; i++)              //����ԭ������
    {
        st_UartTx.TxDataBuff[i] = st_UartRec.RxDataBuff[i];
    }        
    st_UartTx.MaxTxNumber = st_UartRec.MaxRxNumber;
}

/**************************************************************************
*    ����ԭ��: void ClearErrorPrc(INT8U Command)
*    ��    ��: �������
**************************************************************************/
void ClearErrorPrc(INT16U Command)
{ 
    if (Command & 0xffff) 
    {
        st_CompRunPar.u32ErroType = 0;
    }
}

/**************************************************************************
*    ����ԭ��: void DecodeCommand(INT8U CommandId)
*    ��    ��: ִ������
**************************************************************************/
void DecodeCommand(INT8U CommandId)
{
    if (CommandId == 0)                     
    {                
            //���ػ�
        if ((SetCommand.SetDataArray[CommandId] & 0x0003) == 0x0000)               
        {
            SetCompressorOff();
        }
        else if ((SetCommand.SetDataArray[CommandId] & 0x0003) == 0x0003)          
        {                
            SetCompressorOn();
        }     
    }
    else if (CommandId == 1)                //�������
    {
        ClearErrorPrc(SetCommand.SetDataArray[CommandId]);          
    }
    else if (CommandId == 2)                //�趨Ŀ��Ƶ��
    {
        SetNewTargrt(SetCommand.SetDataArray[CommandId]); 
    }
}

/**************************************************************************
*    ����ԭ��: void WriteData(INT16U Address, INT16U Data)
*    ��    ��: 
**************************************************************************/
void WriteData(INT16U TargetAddress, INT16U Data)
{
    INT8U Offset = 0;
        
    if ((TargetAddress >= WRITE_COMMAND_START_ADDRESS) && (TargetAddress <= WRITE_COMMAND_END_ADDRESS))
    {       //--д����
        Offset = (TargetAddress - WRITE_COMMAND_START_ADDRESS);
        
        SetCommand.SetDataArray[Offset] = Data;

        if(st_CompRunPar.u16MotorInputMode == COMM_INPUT_MODE)
        {
            DecodeCommand(Offset);                // 
        }
    }
    else if ((TargetAddress >= WRITE_EEPROM_START_ADDRESS) && (TargetAddress <= WRITE_EEPROM_END_ADDRESS))
    {
        //--дEEPROM
        Offset = (TargetAddress - WRITE_EEPROM_START_ADDRESS);
        
        EepromParameter.DataArray[Offset] = Data;  
     
        UpdateEeprom(Offset,Data);           //modbus��ַ����һλ����������EEPROM��ַ
    }
}

/**************************************************************************
*    ����ԭ��: void WriteSingle(void)
*    ��    ��: 
**************************************************************************/
void WriteSingle(void)
{
    INT16U TargetAddress = 0;
    INT16U Data = 0;
    
    TargetAddress = ((st_UartRec.RxDataBuff[2] << 8) | st_UartRec.RxDataBuff[3]);
    
    Data = ((st_UartRec.RxDataBuff[4] << 8) | st_UartRec.RxDataBuff[5]);                                 
    WriteData(TargetAddress,Data);        
}
/**************************************************************************
*    ����ԭ��: void WriteMulti(void)
*    ��    ��: 
**************************************************************************/
void WriteMulti(void)
{
    INT16U TargetAddress = 0;
    INT16U  Data = 0;
    INT8U  WriteByteNum = 0;
    INT8U  ByteId = 0;
    INT8U  i = 0;
    
    TargetAddress = ((st_UartRec.RxDataBuff[2] << 8) | st_UartRec.RxDataBuff[3]);
    WriteByteNum = st_UartRec.RxDataBuff[5];                                      //ȡҪд�ļĴ�����ʽ                   
    
    ByteId = 7;                                                     //��st_UartRec.RxDataBuff[7] ��ʼ��������
    for (i = 0; i < WriteByteNum; i++)
    {
        Data = st_UartRec.RxDataBuff[ByteId] << 8;                            //ȡ���ֽ� 
        ByteId++;
        Data |= st_UartRec.RxDataBuff[ByteId];                                //ȡ���ֽ�         
        ByteId++;
        
        WriteData(TargetAddress,Data);
        TargetAddress++;
    }
}
/**************************************************************************
*    ����ԭ��: void ReadyWritMultiAckData(void)
*    ��    ��: 
**************************************************************************/
void ReadyWriteMultiAckData(void)
{
    INT8U i = 0;
    INT16U CheckSum = 0;

    st_UartTx.TxDataBuff[0] = st_UartRec.RxDataBuff[0];
    st_UartTx.TxDataBuff[1] = st_UartRec.RxDataBuff[1];
    st_UartTx.TxDataBuff[2] = st_UartRec.RxDataBuff[6];                //д���ֽ���
    
    st_UartTx.TxDataBuff[3] = st_UartRec.RxDataBuff[2];
    st_UartTx.TxDataBuff[4] = st_UartRec.RxDataBuff[3];                //��ʼ��ַ
    
    st_UartTx.TxDataBuff[5] = st_UartRec.RxDataBuff[4];
    st_UartTx.TxDataBuff[6] = st_UartRec.RxDataBuff[5];                //д��Ĵ�����Ŀ
    
    //--���������ж�������У���
    if ((st_UartRec.RxDataBuff[1]& 0x80) == 0)
    {       //CRCУ��
      
        CheckSum = CRC16(&st_UartTx.TxDataBuff[0],7);        
        st_UartTx.TxDataBuff[7] = CheckSum >>8;     //��λ
        st_UartTx.TxDataBuff[8] = CheckSum ;        //��λ
    }
    else
    {
        for (i = 0; i < 7; i++)
        {
            CheckSum += st_UartTx.TxDataBuff[i];          //Address����ʱ����
        }
        
        st_UartTx.TxDataBuff[7] =(INT8U) (CheckSum >> 8);               
        st_UartTx.TxDataBuff[8] =(INT8U) (CheckSum & 0x00ff);
    }     
    
    st_UartTx.MaxTxNumber = 9;
}
/**************************************************************************
*    ����ԭ��: void WriteNewCheckSum(void)
*    ��    ��: ���¼���У��Ͳ����µ�У���д��EEPROM��
**************************************************************************/
void WriteNewCheckSum(void)
{	 
    INT16U CheckSum = 0;	
	INT16U tmp = 0;
	INT16U CheckTab = 0;
	
	//--crcУ��	
	if (g_CrcCheck)
	{
	    CheckSum = CRC16(&EepromParameter.Data8UArray[0],((g_ReadDataNum - 1) << 1));
	}
	else
	{
	    CheckSum = GetAccumulateSum(&EepromParameter.Data8UArray[0],((g_ReadDataNum - 1)<< 1));
	}	
	
    EepromParameter.Data.CheckSum = CheckSum;             //ע�⣬�ߵ��ֽ�˳��ͬ������ֱ�Ӹ�ֵ
	CheckTab = ((EEPROM_WORD_NUM) << 1) - 1;
	tmp = (EepromParameter.Data8UArray[CheckTab] << 8)| EepromParameter.Data8UArray[(CheckTab -1)];	//���⴦��
	 
	
	WriteData(CHECKSUM_ADDRESS,tmp);				//����EEPROM�е�У���,ע�����߼���ַ
	 
}

/**************************************************************************
*    ����ԭ��: void DecodeRx(void)
*    ��    ��: 
**************************************************************************/
void DecodeUartRx(void)
{
    INT16U CheckValue = 0;
    
    if(st_UartRec.OneFrameRecFlag == 1)
    {
        st_UartRec.OneFrameRecFlag = 0;
        CheckValue = CheckSumPrc(); 
        if(CheckValue == 1)                      //У������ȷ
        {
            if ((st_UartRec.RxDataBuff[1] & 0x7f) == MODBUS_READ)
            {
                ReadyReadCmdAckData();
            }
            else if ((st_UartRec.RxDataBuff[1] & 0x7f) == MODBUS_WRITE_SINGLE)
            {                
                WriteSingle();
                WriteNewCheckSum();			//д���µ�У���
                ReadyWriteSingleAckData();
            }
            else if ((st_UartRec.RxDataBuff[1] & 0x7f) == MODBUS_WRITE_MULTI)
            {                         
                WriteMulti();
                WriteNewCheckSum();			//д���µ�У���
                ReadyWriteMultiAckData();
            }
            else if ((st_UartRec.RxDataBuff[1] & 0x7f) == MODBUS_WRITE_AND_READ)
            {
                    
            }

            
            st_UartTx.DelayTxFlag = 1;
            st_UartTx.TxDelayTime = 0;           
            
        }
        else
        {
            FM0P_MFS0_UART->SCR_f.RXE = 1;   //en rx
            FM0P_MFS0_UART->SCR_f.RIE = 1;   // en rx int 
        }  
    }
}

/**************************************************************************
*    ����ԭ��: void ISR_UART0_Receive(void)
*    ��    ��:
**************************************************************************/
void ISR_UART0_Receive(void)
{
    INT8U UartData;          

    UartData = FM0P_MFS0_UART->RDR;                    //��ȡ����  
    
    if(FM0P_MFS0_UART->SSR & 0x38)      //Receive Error Action:PE,FRE,ORE
    {                                       	
        FM0P_MFS0_UART->SSR |= 0x80;        //REC=1:clear error(PE,FRE,ORE) flag\
              
        return;
    }

    st_UartRec.RxDataBuff[st_UartRec.RxCount] = UartData;
    st_UartRec.RxCount++;
    if(st_UartRec.RxCount >= MAX_DEF_RX_BYTE)
    {
        st_UartRec.RxCount = 0;
    } 

    FM0P_BT3_RT->TMCR &= 0xfffC;      //Disable software trigger and count stop  
    /*������ʱ�������������4ms�����ݣ���˵��һ֡���ݴ��ͽ���*/   
    FM0P_BT3_RT->STC &= 0xF8;        //Enable underflow interrupt,Clear interupt cause
    FM0P_BT3_RT->PCSR = 78;          // 4MS    
    FM0P_BT3_RT->TMR = 78;
    FM0P_BT3_RT->TMCR|= 0x03;      //software trigger and count start 
}
/**************************************************************************
*    ����ԭ��: void ISR_UART3_Transmit(void)
*    ��    ��:
**************************************************************************/
void ISR_UART0_Transmit(void)
{       
    if (st_UartTx.TxCount >= st_UartTx.MaxTxNumber)
    {
        st_UartTx.TxCount = 0;
         
        FM0P_MFS0_UART->SCR_f.TIE = 0;
        FM0P_MFS0_UART->SCR_f.TXE = 0;
        FM0P_MFS0_UART->SCR_f.RXE = 1;   //en rx
        FM0P_MFS0_UART->SCR_f.RIE = 1;   // en rx int 
     
    }
    else
    {
        FM0P_MFS0_UART->RDR = st_UartTx.TxDataBuff[st_UartTx.TxCount];
        st_UartTx.TxCount++;
    }
}
/**************************************************************************
*    ����ԭ��: void UartFrameRec(void)
*    ��    ��:
**************************************************************************/
void UartFrameRec(void)
{    
    FM0P_BT3_RT->STC &= 0xF8;        //Enable underflow interrupt,Clear interupt cause

    FM0P_BT3_RT->TMCR &= 0xfffC;      //Disable software trigger and count stop 
   
    if(st_UartRec.RxDataBuff[0] == MODBUS_ADDRESS)
    {
        st_UartRec.MaxRxNumber = st_UartRec.RxCount;
        st_UartRec.OneFrameRecFlag = 1;
        st_UartRec.RxCount = 0;
        FM0P_MFS0_UART->SCR_f.RXE = 0;   //Disable rx
        FM0P_MFS0_UART->SCR_f.RIE = 0;   // Disable rx int 
    }
    else
    {
        st_UartRec.RxCount = 0;
    }

}

