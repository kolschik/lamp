typedef enum
{
    ir_state_reset,
    ir_state_start,
    ir_state_rx,
    ir_state_ready = 66,
    ir_state_repeat
} ir_state_type;

void work (uint8_t);
void refresh ();