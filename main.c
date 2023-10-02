#include "mcc_generated_files/mcc.h"
#include "main.h"
#include "setup.h"

volatile unsigned long ir_data;
volatile unsigned char ir_aval = 0, flash_pointer, delay_mem;

unsigned char bright, power = 0x0F;
unsigned char cfg_mode;
ir_state_type ir_state=0;
//    9 13 18 25 35 49 68 94 131 183 255
const uint8_t pf[BRIGHT_MAX+1] @ 0x10  = {0,4,6,9,12,16,21,28,37,49,64,84,111,146,192,255};//{1,2,4,6,8,11,16,23,32,45,64,91,128,181,254};
//                            0 1 2 3 4 5 6  7  8  9  10 11 12 13  14  15  16
const uint8_t ma[16] @ ma_addr  = {BRIGHT_DEF, 0xff};

void main(void)
{
    uint8_t delay_blink;
    SYSTEM_Initialize();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
     
    bright = ma[0];
    
    while (1)
    {
        CLRWDT();
        uint8_t last, delay_repeat, repeat_reject;
        if (ir_aval == 1) //ѕрин€та основна€ команда
        {   
            //моргаем светодиодом
            delay_blink = 0;    
            LATAbits.LATA1 = 0;
            //провер€ем наш ли пульт
            uint8_t dev;
            dev = ir_data >> 24;
            if (dev == REM_DEV)
            {
                //пульт совпал, проверка команды
                uint8_t cmd, cmd_;
                cmd = ir_data >> 8;
                cmd_ = ~ir_data;
                if ( cmd == cmd_)
                {
                    //посылка правильна€
                    last = cmd;
                    work (last); 
                    //сбрасываю таймаут между посылкми
                        repeat_reject = 0;                    
                }
            }
            //очищаем флаг приема     
            INTERRUPT_GlobalInterruptDisable();
            ir_aval = 0;
            INTERRUPT_GlobalInterruptEnable(); 
            //сбрасываем контроль повторов
            delay_repeat = 0; 
            
        }
       if (ir_aval == 2) //прин€т повтор
        { 
            //моргаем светодиодом
            delay_blink = 0;  
            LATAbits.LATA1 = 0; 
            //сбрасываю таймаут между посылкми
            repeat_reject = 0;
            //провер€ю, прошел ли таймаут задержки
            if (delay_repeat >= IR_DELAY) work (last);
            else delay_repeat++;
                
           /* if (cfg_mode > 0)
            {
                if (cfg_mode < CFG) cfg_mode++;
                else power = 0;
            }*/
            //очищаем флаг приема
            INTERRUPT_GlobalInterruptDisable();
            ir_aval = 0;
            INTERRUPT_GlobalInterruptEnable();
        
        }  
        //гасим светодиод
        if (delay_blink < BLINK_DELAY) delay_blink++;     
        else LATAbits.LATA1 = 1;
        
        if (repeat_reject++ > REPEAT_REJECT) {
            last = REM_DEV;
        }
        
        //сохранение в пам€ть
        if (delay_mem <= MEM_DELAY) delay_mem++; 
        if ((delay_mem == MEM_DELAY) && save_mem)
        {
           
            if (bright != ma[0]) 
            { 
                CLRWDT();
                INTCONbits.GIE = 0; // Disable interrupts
                // Load lower 8 bits of erase address boundary
                PMADRL = (ma_addr & 0xFF);
                // Load upper 6 bits of erase address boundary
                PMADRH = ((ma_addr & 0xFF00) >> 8);
                // Block erase sequence
                PMCON1bits.CFGS = 0;    // Deselect Configuration space
                PMCON1bits.FREE = 1;    // Specify an erase operation
                PMCON1bits.WREN = 1;    // Allows erase cycles

                // Start of required sequence to initiate erase
                PMCON2 = 0x55;
                PMCON2 = 0xAA;
                PMCON1bits.WR = 1;      // Set WR bit to begin erase
                NOP();
                NOP();

                PMCON1bits.WREN = 0;       // Disable writes
                    // Block write sequence
                PMCON1bits.CFGS = 0;    // Deselect Configuration space
                PMCON1bits.WREN = 1;    // Enable wrties
                PMCON1bits.LWLO = 1;    // Only load write latches

                unsigned char i;
                for (i=0; i<WRITE_FLASH_BLOCKSIZE; i++)
                {
                    // Load lower 8 bits of write address
                    PMADRL = ((ma_addr + i) & 0xFF);
                    // Load upper 6 bits of write address
                    PMADRH = ((ma_addr & 0xFF00) >> 8);
                     // Load data in current address
                    PMDATL = bright;
                    PMDATH = 0x34;

                    if(i == (WRITE_FLASH_BLOCKSIZE-1)) PMCON1bits.LWLO = 0; // Start Flash program memory write

                    PMCON2 = 0x55;
                    PMCON2 = 0xAA;
                    PMCON1bits.WR = 1;
                    NOP();
                    NOP();
                    CLRWDT();
                }

                PMCON1bits.WREN = 0;       // Disable writes
                INTCONbits.GIE = 1;	// Restore interrupt enable               
            }
        }
        refresh();
        __delay_ms(3);
    }   
}


void refresh ()
{
    static uint16_t  pwm_now=0;
    uint16_t pwm_target;
    //высчитываю требуемую €ркость
    bright & = BRIGHT_MAX;
    pwm_target = pf[bright & power]<<1;
    //сравниваю с текущей
    if (pwm_target != pwm_now)
    {
        if (pwm_target > pwm_now) pwm_now++;
        else pwm_now--;
        //загружаю в шим
        PWM1_LoadDutyValue(pwm_now);
    }
}

void work (uint8_t task)
{
    power = 0x0F;
    delay_mem =0;
    switch(task)
    {
        case but_up:
        if ( bright < BRIGHT_MAX) bright++;
        break;
        
        case but_dw:
        if ( bright > BRIGHT_MIN) bright--;
        break;
        
        case but_off:
            power = 0;
            break;
        
        case but_on:
            if (cfg_mode == 0) cfg_mode++;
            break;
            
        case but_RED:
            bright = 1;
            break;
            
        case but_GRN:
            bright = 6;
            break; 
            
        case but_BLU:
            bright = 12;
            break;  
            
        case but_WHT:
            bright = 15;
            break;
        default:
            break;
    }   
}