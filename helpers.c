#include<stdio.h>
#include<stdint.h>
#include"helpers.h"

void helper_write_from_file(char *fname, uint8_t *mem_ptr, uint64_t size){
    FILE *fptr  = fopen(fname, "rb");
    if(fptr == NULL){
        printf("Unable to open file\n");
        exit(-1);
    }

    fseek(fptr, 0, SEEK_SET);
    uint64_t fsize = helper_get_file_size(fname);

    fread(mem_ptr, 1, size > fsize ? fsize : size, fptr);
    fclose(fptr);
}

uint64_t helper_get_file_size(char *fname){
    FILE *fptr  = fopen(fname, "rb");
    if(fptr == NULL){
        printf("Unable to open file\n");
        exit(-1);
    }

    fseek(fptr, 0, SEEK_END);
    uint64_t fsize = ftell(fptr);
    fclose(fptr);
    return fsize;
}