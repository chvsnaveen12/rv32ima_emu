#include<stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include"helpers.h"

// #define RISCOF

int main(int32_t argc, char **argv){
#ifndef RISCOF
    if(argc != 5){
        printf("Give SBI, linux, Initrd and DTB file\n");
        exit(-1);
    }
    soc_init(argv[1], argv[2], argv[3], argv[4]);
    soc_run();
#else
    uint32_t sig_begin, sig_end;
    if(argc != 4){
        printf("File and params???\n");
        exit(-1);
    }

    sig_begin = strtoul(argv[2], (char**)NULL, 16);
    sig_end = strtoul(argv[3], (char**)NULL, 16);

    soc_init(argv[1], argv[1], argv[1], argv[1]);
    soc_run();
    core_memory_dump(sig_begin, sig_end, false);
#endif

}