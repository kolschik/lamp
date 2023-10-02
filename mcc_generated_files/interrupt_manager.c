#include "interrupt_manager.h"
#include "mcc.h"
#include "..\main.h"
extern volatile unsigned long ir_data;
extern volatile unsigned char ir_aval;

extern ir_state_type ir_state;
void __interrupt INTERRUPT_InterruptManager (void)
{
    if((INTCONbits.IOCIE == 1) && (INTCONbits.IOCIF == 1))
    {
        
        unsigned char TMR_S = TMR0;
        TMR0=0; //41 uS
        IOCAF = 0;
        if ((TMR_S > 125) & (TMR_S < 155)) ir_state = ir_state_start; 
        else switch (ir_state)
        {
            case ir_state_reset:  
                break;
                
            case ir_state_start: 
                if ((TMR_S > 62) & (TMR_S < 73))ir_state = ir_state_rx;
                else if ((TMR_S > 30) & (TMR_S < 40))ir_state=ir_state_repeat;  
                else ir_state=ir_state_reset;  
                break;   
                
            case ir_state_repeat:
                if ((TMR_S > 6) && (TMR_S < 10)) ir_aval = 2;
                ir_state=ir_state_reset;     
                break; 
                
            default:
            if (0x01 & ir_state++)  
            {
                if (TMR_S > 45) ir_state=ir_state_reset;   
                else if (TMR_S > 10) ir_data |= 1;
                if (ir_state == ir_state_ready) 
                {
                    ir_aval = 1;
                    ir_state = ir_state_reset;         
                }           
            }
            else
            {
                if ((TMR_S < 7) | (TMR_S > 9)) ir_state=ir_state_reset;
                ir_data = (ir_data << 1);
            }
            break;
        }
        IOCAFbits.IOCAF3 = 0; 
    }
    
    else if(INTCONbits.TMR0IE == 1 && INTCONbits.TMR0IF == 1)
    {
        INTCONbits.TMR0IF = 0; 
        ir_state=ir_state_reset;
    }
    else
    {
        //Unhandled Interrupt
    }
}
