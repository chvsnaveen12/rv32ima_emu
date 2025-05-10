#include<stdio.h>
#include<unistd.h>

#include"helpers.h"

#ifdef RISCOF
    #define TIMES 20000
#else
    #define TIMES 100000000000
#endif

bool temp = false;

void soc_init(char* kernel_file, char* linux_file, char* initrd_file, char* dtb_file){
    memory_init(kernel_file, linux_file, initrd_file, dtb_file);
    core_init();
    uart_init();
    clint_set_mtime(0xffffffffffffffffUL);
}

uint64_t counter = 0;

// void temp(){

// }

void soc_run(){
    uint32_t inst;
    // for(uint64_t i = 0; i < 200000000; i++){
    for(uint64_t i = 0; i < 100000000000; i++){
        bool mei = 0, sei = 0, msi = 0, mti = 0;
        if(core_fetch(&inst)){
            // if(inst == 0x00100073){
            //     core_dump();
            //     exit(-1);
            //     break;
            // }
            core_decode(inst);
            core_execute();

        }
        // if(mti)
        //     temp();
        bool uart_irq = uart_update();
        bool blk_interrupt = blk_int();
        plic_update_pending(10, uart_irq);
        plic_update_pending(3, blk_interrupt);
        plic_update(&mei, &sei);

        clint_update(&msi, &mti);
        core_process_interrupts(mei, sei, mti, msi);
        // if(counter++ > 20000){
        // if(counter++ > 700000000){
            if(temp){
            core_dump();
        // //     temp();
            usleep(100000);
        }

        // if(counter > 2286682)
            // getchar();
            // usleep(1000);
    }
}
