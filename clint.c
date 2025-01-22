#include<stdio.h>
#include<string.h>
#include"helpers.h"

#define MSIP_OFFSET 0x0
#define MSIP_SIZE 0x04

#define MTIME_CMP_OFFSET 0x4000
#define MTIME_CMP_SIZE 0x08

#define MTIME_OFFSET 0xBFF8
#define MTIME_SIZE 0x08

struct clint{
    uint32_t msip;
    uint64_t mtimecmp;
    uint64_t mtime;
    uint64_t cycle;
} clint;

bool clint_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    (void) priv;
    uint32_t tmp_addr;
    uint8_t* u8_ptr;

    if(access == bus_write){
        if(MSIP_OFFSET <= addr && (addr + len) <= MSIP_OFFSET + MSIP_SIZE){
            tmp_addr = addr-MSIP_OFFSET;
            u8_ptr = (uint8_t*)&clint.msip;
        }
        else if(MTIME_CMP_OFFSET <= addr && (addr + len) <= MTIME_CMP_OFFSET + MTIME_CMP_SIZE){
            tmp_addr = addr-MTIME_CMP_OFFSET;
            u8_ptr = (uint8_t*)&clint.mtimecmp;
        }
        else if(MTIME_OFFSET <= addr && (addr + len) <= MTIME_OFFSET + MTIME_SIZE){
            tmp_addr = addr-MTIME_OFFSET;
            u8_ptr = (uint8_t*)&clint.mtime;
        }
        else
            return false;
        memcpy(&u8_ptr[tmp_addr], val, len);
    }
    else{
        if(MSIP_OFFSET <= addr && (addr + len) <= MSIP_OFFSET + MSIP_SIZE){
            tmp_addr = addr-MSIP_OFFSET;
            u8_ptr = (uint8_t*)&clint.msip;
        }
        else if(MTIME_CMP_OFFSET <= addr && (addr + len) <= MTIME_CMP_OFFSET + MTIME_CMP_SIZE){
            tmp_addr = addr-MTIME_CMP_OFFSET;
            u8_ptr = (uint8_t*)&clint.mtimecmp;
        }
        else if(MTIME_OFFSET <= addr && (addr + len) <= MTIME_OFFSET + MTIME_SIZE){
            tmp_addr = addr-MTIME_OFFSET;
            u8_ptr = (uint8_t*)&clint.mtime;
        }
        else
            return false;
        memcpy(val, &u8_ptr[tmp_addr], len);
    }
    return true;
}

void clint_set_mtime(uint64_t val){
    clint.mtime = val;
}

uint64_t clint_get_mtime(){
    return clint.mtime;
}

void clint_update(bool* msi, bool* mti){
    if(clint.cycle++ % 2 == 0)
        clint.mtime++;

    *mti = clint.mtime >= clint.mtimecmp;
    *msi = clint.msip & 0x01;
}