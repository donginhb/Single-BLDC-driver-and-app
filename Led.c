
/******************************************************************************/
/** \file led.c
 **
 ** Add description here...
 **
 ** History:
 **   
 *****************************************************************************/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#include "InitMcu.h"
#include "system_s6e1a1.h"
#include "s6e1a1.h"
#include "base_types.h"
#include "Led.h"

Led_Para st_LedPara;

/**************************************************************************
*    ����ԭ��:void InitLedPort(void)
*    ��    ��:
**************************************************************************/
void InitLedPort(void)
{
    //--LED1    
    FM0P_GPIO->PFR6_f.P61 = 0;
    FM0P_GPIO->DDR6_f.P61 = 1;                // output pin
    FM0P_GPIO->PDOR6_f.P61 = 1;	
}
/**************************************************************************
*    ����ԭ��:void DispCode(INT8U Code)
*    ��    ��: 
**************************************************************************/
void DispCode(INT8U Code)
{
    //--�����趨����
    INT8U i = 0;
    INT8U j = 0;

    i = (Code >> 4) << 1;           //
    j = (Code & 0x0f) << 1;         //�ߵ���ֵ�����0.5SΪ��λ ��12 = 2S��4S

    st_LedPara.DispPeriodSecond = j + 8;       //��˸����= ��˸ʱ��+2S����ʱ��
    // DispPeriodSecond = 28;       //��˸����= 7S

    #if 0
    //--ʮλ
    if (st_LedPara.DispTenTime >= i)
    {
        SwitchOffLed1();
    }
    else
    {
        if ((st_LedPara.DispTenTime & 0x01) == 0)
        {
            SwitchOnLed1();
        }
        else
        {
            SwitchOffLed1();
        }
    }
#endif
    //--��λ
    if (st_LedPara.DispUnitTime >= j)
    {
        SwitchOffLed1();
    }
    else
    {
        if ((st_LedPara.DispUnitTime & 0x01) == 0)
        {
            SwitchOnLed1();
        }
        else
        {
            SwitchOffLed1();
        }
    }
}

/**************************************************************************
*    ����ԭ��:void DispAlarm(void)
*    ��    ��:��������ʾ
**************************************************************************/
void DispAlarm(void)
{

    if (st_CompRunPar.u32ErroType == UNDER_VOLTAGE)        //under voltage
    {
        DispCode(0x03);
    }
    else if (st_CompRunPar.u32ErroType == OVER_VOLTAGE)        //over voltage
    {
        DispCode(0x02);
    }
    else if (st_CompRunPar.u32ErroType == MOTOR_LOCK)        //motor ��ת
    {
        DispCode(0x06);
    }
    else if (st_CompRunPar.u32ErroType == SW_OVER_CURRENT)        //hardware over current
    {
        DispCode(0x04);
    }  
    else if (st_CompRunPar.u32ErroType == AD_MIDDLE_ERROR)        
    {
        DispCode(0x01);
    }   
    else if (st_CompRunPar.u32ErroType == BOARD_SENSOR_ERROR)        
    {
        DispCode(0x05);
    }   
    else if (st_CompRunPar.u32ErroType == SPEED_ERROR)        
    {
        DispCode(0x07);
    }  
    else if (st_CompRunPar.u32ErroType == MOTOR_OVER_CURRENT)        
    {
        DispCode(0x08);
    }  
	
}
/**************************************************************************
*    ����ԭ��:void DispNormalRunning(void)
*    ��    ��:ָʾ����˸
**************************************************************************/
void DispNormalRunning(void)
{
    if (st_LedPara.F_NormalFlashOn)
    {
        SwitchOnLed1();
       
    }
    else
    {
        SwitchOffLed1();
        
    }
}

/**************************************************************************
*    ����ԭ��:void LedDisp(void)
*    ��    ��:ָʾ����˸
**************************************************************************/
void LedDisp(void)
{     
    if (st_CompRunPar.u32ErroType == 0)               //Ŀǰ�õ��Ĺ��ϱ�־
    {
        DispNormalRunning();
    }
    else
    {        
        DispAlarm();                
    }    
      
}


