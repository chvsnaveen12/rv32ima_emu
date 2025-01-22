#include<stdio.h>
#include"helpers.h"

#define PRIO_OFFSET (0x0 >> 2)
#define PRIO_SIZE (0x400 >> 2)

#define PENDING_OFFSET (0x1000 >> 2)
#define PENDING_SIZE (0x20 >> 2)

#define ENABLE0_OFFSET (0x2000 >> 2)
#define ENABLE0_SIZE (0x20 >> 2)

#define ENABLE1_OFFSET (0x2080 >> 2)
#define ENABLE1_SIZE ENABLE0_SIZE

#define PRIO0_THRESH_OFFSET (0x200000 >> 2)
#define PRIO0_THRESH_SIZE (0x04 >> 2)

#define PRIO1_THRESH_OFFSET (0x201000 >> 2)
#define PRIO1_THRESH_SIZE (0x04 >> 2)

#define CLAIM0_COMPLETE_OFFSET (0x200004 >> 2)
#define CLAIM0_COMPLETE_SIZE (0x04 >> 2)

#define CLAIM1_COMPLETE_OFFSET (0x201004 >> 2)
#define CLAIM1_COMPLETE_SIZE CLAIM0_COMPLETE_SIZE

#define MAX_INTS 256
#define PRIO_REGS 256
#define PENDING_REGS 8
#define ENABLE_REGS 8
#define CLAIMED_REGS 8

struct plic{
    uint32_t pending[PENDING_REGS];
    uint32_t priority[PRIO_REGS];

    uint32_t enable0[ENABLE_REGS];
    uint32_t enable1[ENABLE_REGS];

    uint32_t claim_complete0;
    uint32_t threshold0;

    uint32_t claim_complete1;
    uint32_t threshold1;

    // Internal
    uint32_t claimed[CLAIMED_REGS];
} plic;

void plic_update_pending(uint32_t id, bool val){
    uint32_t reg = id/32;
    uint32_t bit = id%32;

    uint32_t temp = plic.pending[reg];
    temp &= ~(1 << bit);
    temp |=  ((uint32_t)val << bit) & ~(plic.claimed[reg] & (1 << bit));
    plic.pending[reg] = temp;
}

void plic_update(bool* mei, bool* sei){
    uint32_t irq_id0 = 0;
    uint32_t irq_prio0 = 0;
    uint32_t irq_id1 = 0;
    uint32_t irq_prio1 = 0;

    for(uint32_t i = 0; i < ENABLE_REGS; i ++){
        if(!plic.enable0[i] || !plic.pending[i])
            continue;
        
        for(uint32_t j = 0; j < (sizeof(plic.enable0[0])*8); j++){
            
            if((plic.enable0[i]  & (1 << j)) && (plic.pending[i] & (1 << j)) && (plic.priority[(i*32) + j] >= plic.threshold0)){
                if(plic.priority[(i*32) + j] > irq_prio0){
                    irq_prio0 = plic.priority[(i*32) + j];
                    irq_id0 = (i*32) + j;
                }
            }
        }
    }

    for(uint32_t i = 0; i < ENABLE_REGS; i ++){
        if(!plic.enable1[i] || !plic.pending[i])
            continue;
        
        for(uint32_t j = 0; j < (sizeof(plic.enable1[0])*8); j++){
            
            if((plic.enable1[i]  & (1 << j)) && (plic.pending[i] & (1 << j)) && (plic.priority[(i*32) + j] >= plic.threshold1)){
                if(plic.priority[(i*32) + j] > irq_prio1){
                    irq_prio1 = plic.priority[(i*32) + j];
                    irq_id1 = (i*32) + j;
                }
            }
        }
    }

    if(irq_prio0 > 0){
        plic.claim_complete0 = irq_id0;
        *mei = true;
    }
    else{
        plic.claim_complete0 = 0;
        *mei = false;
    }

    if(irq_prio1 > 0){
        plic.claim_complete1 = irq_id1;
        *sei = true;
    }
    else{
        plic.claim_complete1 = 0;
        *sei = false;
    }

}

bool plic_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    (void) priv;

    if(addr % 4 != 0 || len != 4){
        printf("PLIC addr misaligned or length != 4, dying uwu\n");
        core_dump();
        exit(-1);
    }

    addr = addr >> 2;

    if(access == bus_write){
        if(PRIO_OFFSET <= addr && addr < PRIO_OFFSET + PRIO_SIZE)
            plic.priority[addr - PRIO_OFFSET] = *val & 0x07;

        else if(PENDING_OFFSET <= addr && addr < PENDING_OFFSET + PENDING_SIZE)
            plic.pending[addr - PENDING_OFFSET] = *val;

        else if(ENABLE0_OFFSET <= addr && addr < ENABLE0_OFFSET + ENABLE0_SIZE)
            plic.enable0[addr - ENABLE0_OFFSET] = *val;
        else if(ENABLE1_OFFSET <= addr && addr < ENABLE1_OFFSET + ENABLE1_SIZE)
            plic.enable1[addr - ENABLE1_OFFSET] = *val;

        else if(PRIO0_THRESH_OFFSET <= addr && addr < PRIO0_THRESH_OFFSET + PRIO0_THRESH_SIZE)
            plic.threshold0 = *val & 0x07;
        else if(PRIO1_THRESH_OFFSET <= addr && addr < PRIO1_THRESH_OFFSET + PRIO1_THRESH_SIZE)
            plic.threshold1 = *val & 0x07;

        else if(CLAIM0_COMPLETE_OFFSET <= addr && addr < CLAIM0_COMPLETE_OFFSET + CLAIM0_COMPLETE_SIZE){
            uint32_t reg = *val / 32;
            uint32_t bit = *val % 32;
            plic.claimed[reg] &= ~(1 << bit);
        }
        else if(CLAIM1_COMPLETE_OFFSET <= addr && addr < CLAIM1_COMPLETE_OFFSET + CLAIM1_COMPLETE_SIZE){
            uint32_t reg = *val / 32;
            uint32_t bit = *val % 32;
            plic.claimed[reg] &= ~(1 << bit);
        }
    }
    else{
        if(PRIO_OFFSET <= addr && addr < PRIO_OFFSET + PRIO_SIZE)
            *val = plic.priority[addr - PRIO_OFFSET];

        else if(PENDING_OFFSET <= addr && addr < PENDING_OFFSET + PENDING_SIZE)
            *val = plic.pending[addr - PENDING_OFFSET];

        else if(ENABLE0_OFFSET <= addr && addr < ENABLE0_OFFSET + ENABLE0_SIZE)
            *val = plic.enable0[addr - ENABLE0_OFFSET];
        else if(ENABLE1_OFFSET <= addr && addr < ENABLE1_OFFSET + ENABLE1_SIZE)
            *val = plic.enable1[addr - ENABLE1_OFFSET];

        else if(PRIO0_THRESH_OFFSET <= addr && addr < PRIO0_THRESH_OFFSET + PRIO0_THRESH_SIZE)
            *val = plic.threshold0;
        else if(PRIO1_THRESH_OFFSET <= addr && addr < PRIO1_THRESH_OFFSET + PRIO1_THRESH_SIZE)
            *val = plic.threshold1;

        else if(CLAIM0_COMPLETE_OFFSET <= addr && addr < CLAIM0_COMPLETE_OFFSET + CLAIM0_COMPLETE_SIZE){
            *val = plic.claim_complete0;
            uint32_t reg = *val / 32;
            uint32_t bit = *val % 32;
            plic.claimed[reg] |= 1 << bit;
        }
        else if(CLAIM1_COMPLETE_OFFSET <= addr && addr < CLAIM1_COMPLETE_OFFSET + CLAIM1_COMPLETE_SIZE){
            *val = plic.claim_complete1;
            uint32_t reg = *val / 32;
            uint32_t bit = *val % 32;
            plic.claimed[reg] |= 1 << bit;
        }
    }
    return true;
}

void plic_dump(){
    printf("Pending: 0x%08x\n", plic.pending[0]);
    printf("PRIO 10: 0x%08x\n", plic.priority[10]);
    printf("PRIO  1: 0x%08x\n", plic.priority[1]);
    printf("Enable0: 0x%08x\n", plic.enable0[0]);
    printf("Enable1: 0x%08x\n", plic.enable1[0]);
    printf("Thresh0: 0x%08x\n", plic.threshold0);
    printf("Thresh1: 0x%08x\n\n", plic.threshold1);

}