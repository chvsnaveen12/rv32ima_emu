#include<stdio.h>
#include<pthread.h>
#include<termios.h>
#include<unistd.h>
#include<string.h>
#include "uart.h"
#include "helpers.h"

extern temp;

struct uart{
    // 16550
    uint8_t dll;
    uint8_t dlm;
    uint8_t lcr;
    uint8_t mcr;
    uint8_t scr;
    uint8_t iir;
    uint8_t ier;
    uint8_t fcr;

    // RX Fifo
    uint8_t fifo_buf[16];
    int64_t fifo_read_ptr;
    int64_t fifo_write_ptr;

    // Sys
    struct termios term_config;
    pthread_mutex_t fifo_lock; 
    uint8_t thre_int;
} uart;

bool print_type = false;


uint8_t uart_rx_fifo_pop(){
    if(uart.fifo_write_ptr - uart.fifo_read_ptr > 0)
        return uart.fifo_buf[uart.fifo_read_ptr++ % 16];
    
    return uart.fifo_buf[uart.fifo_read_ptr % 16];
}

void uart_rx_fifo_push(uint8_t val){
    if(uart.fifo_write_ptr - uart.fifo_read_ptr < 16)
        uart.fifo_buf[uart.fifo_write_ptr++ % 16] = val;
    else{
        uart.fifo_buf[uart.fifo_write_ptr++ % 16] = val;
        uart.fifo_read_ptr++;
    }
}

int64_t uart_get_size(){
    return uart.fifo_write_ptr - uart.fifo_read_ptr;
}


void* uart_rx_thread(void* ptr){
    unsigned char temp_buf;
    while(true){
        temp_buf = getchar();
        if(temp_buf == 1){
            bool mei, sei;
            printf("CTRL+A pressed, dying uwu\n");
            // uart_dump();
            // plic_update(&mei, &sei);
            // printf("\nMEI:%d, SEI:%d\n", mei, sei);
            // plic_dump();
            // core_dump();
            exit(0);
        }
        else if(temp_buf == 22){
            print_type = !print_type;
            temp = !temp;
        }
        // if(print_type)
        pthread_mutex_lock(&uart.fifo_lock);
        uart_rx_fifo_push(temp_buf);
        pthread_mutex_unlock(&uart.fifo_lock);
    }
}

void uart_revert(void){
    uart.term_config.c_lflag |= (ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(0, TCSANOW, &uart.term_config);
}

void uart_init(void){
    pthread_t thread_id;
    memset(&uart, 0, sizeof(uart));
    pthread_mutex_init(&uart.fifo_lock, NULL);
    tcgetattr(0, &uart.term_config);
    uart.term_config.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(0, TCSANOW, &uart.term_config);
    atexit(uart_revert);
    pthread_create(&thread_id, NULL, uart_rx_thread, NULL);
}

bool uart_update(){
    int64_t rx_int_threshold = 1;
    uint8_t rx_int;
    uint8_t tx_int;
    
    uart.fcr &= ~(0b100);

    if(uart.fcr & 0b10){
        uart.fifo_read_ptr = uart.fifo_write_ptr;
        uart.fcr &= ~(0b10);
    }

    // if(uart.fcr & 1){
    //     switch (uart.fcr >> 6){
    //         case 0b00:
    //             rx_int_threshold = 1;
    //             break;
    //         case 0b01:
    //             rx_int_threshold = 4;
    //             break;
    //         case 0b10:
    //             rx_int_threshold = 8;
    //             break;
    //         case 0b11:
    //             rx_int_threshold = 14;
    //             break;
    //         default:
    //             rx_int_threshold = 1;
    //             break;
    //     }
    // }
    // else{
    //     rx_int_threshold = 1;
    // }

    rx_int = uart_get_size() >= rx_int_threshold ? 1 : 0;

    rx_int = rx_int & uart.ier;
    tx_int = (uart.thre_int << 1) & uart.ier;

    if(rx_int)
        uart.iir = 0b0100;
    else if(tx_int)
        uart.iir = 0b0010;
    else
        uart.iir = 1;

    uart.iir |= uart.fcr & 1 ? 0b11000000 : 0;
    return !(uart.iir & 1);
}

bool uart_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    (void)priv;

    if(len != 1){
        printf("Uart doesn't support multi-byte bus access, dying uwu\n");
        exit(-1);
    }

    if(access == bus_write){
        switch(addr){
            case W_THR_DLL:
                if(uart.lcr & 0x80){
                    uart.dll = *val;
                }
                else{
                    if(print_type)
                        printf("%d\n", (uint8_t)*val);
                    else
                        putchar((uint8_t)*val);
                    fflush(stdout);
                    uart.thre_int = 1;
                }
                break;
            case W_IER_DLM:
                if(uart.lcr & 0x80){
                    uart.dlm = *val;
                }
                else{
                    uart.ier = *val;
                }
                break;
            case W_FCR:
                uart.fcr = *val;
                break;
            case W_LCR:
                uart.lcr = *val;
                break;
            case W_MCR:
                uart.mcr = *val;
                break;
            case W_SCR:
                uart.scr = *val;
                break;
            default:
                printf("Write Invalid UART Addr: 0x%08x, dying uwu\n", addr);
                exit(-1);
                break;
        }
    }
    switch(addr){
            case R_RBR_DLL:
                if(uart.lcr & 0x80){
                    *val = uart.dll;
                }
                else{
                    *val = uart_rx_fifo_pop();
                }
                break;
            case R_IER_DLM:
                if(uart.lcr & 0x80){
                    *val = uart.dlm;
                }
                else{
                    *val = uart.ier;
                }
                break;
            case R_IIR:
                *val = uart.iir;
                uart.thre_int = 0;
                break;
            case R_LCR:
                *val = uart.lcr;
                break;
            case R_MCR:
                *val = uart.mcr;
                break;
            case R_LSR:
                *val = UART_LSR | (uart_get_size() > 0 ? 1 : 0);
                break;
            case R_MSR:
                *val = UART_MSR;
                break;
            case R_SCR:
                *val = uart.scr;
                break;
            default:
                printf("Read Invalid UART Addr: 0x%08x, dying uwu\n", addr);
                exit(-1);
                break;
        }
    return true;
}

void uart_dump(){
    printf("UART DUMP\n");
    printf("DLL: 0x%02x, DLM: 0x%02x, IER: 0x%02x, FCR: 0x%02x\n", uart.dll, uart.dlm, uart.ier, uart.fcr);
    printf("IIR: 0x%02x, LCR: 0x%02x, MCR: 0x%02x, LSR: 0x%02x\n", uart.iir, uart.lcr, uart.mcr, UART_LSR | (uart_get_size() > 0 ? 1 : 0));
    printf("MSR: 0x%02x, SCR: 0x%02x\n", UART_MSR, uart.scr);
    printf("buf00: 0x%02x, buf01: 0x%02x, buf02: 0x%02x, buf03: 0x%02x\n", uart.fifo_buf[0], uart.fifo_buf[1], uart.fifo_buf[2], uart.fifo_buf[3]);
    printf("buf04: 0x%02x, buf05: 0x%02x, buf06: 0x%02x, buf07: 0x%02x\n", uart.fifo_buf[4], uart.fifo_buf[5], uart.fifo_buf[6], uart.fifo_buf[7]);
    printf("buf08: 0x%02x, buf09: 0x%02x, buf10: 0x%02x, buf11: 0x%02x\n", uart.fifo_buf[8], uart.fifo_buf[9], uart.fifo_buf[10], uart.fifo_buf[11]);
    printf("buf12: 0x%02x, buf13: 0x%02x, buf14: 0x%02x, buf15: 0x%02x\n", uart.fifo_buf[12], uart.fifo_buf[13], uart.fifo_buf[14], uart.fifo_buf[15]);
    printf("Read_ptr: %ld, Write_ptr: %ld\n", uart.fifo_read_ptr, uart.fifo_write_ptr);

    uint32_t mie;
    csr_read(machine, 0x304, &mie);
    printf("MIE: 0x%08x\n", mie);
}