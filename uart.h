#include<stdint.h>
#include<stdbool.h>

#define FIFO_PTR_MASK 0x0f

#define R_RBR_DLL   0
#define W_THR_DLL   0
#define R_IER_DLM   1
#define W_IER_DLM   1
#define R_IIR       2
#define W_FCR       2
#define R_LCR       3
#define W_LCR       3
#define R_MCR       4
#define W_MCR       4
#define R_LSR       5
#define R_MSR       6
#define R_SCR       7
#define W_SCR       7

#define UART_MSR 0x10
#define UART_LSR 0x60