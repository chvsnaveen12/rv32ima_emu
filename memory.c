#include<stdio.h>
#include<string.h>
#include "helpers.h"
#include "csr.h"
#include "memory.h"

#define RAM_SIZE 1024*1024*128

uint8_t __attribute__((aligned (4)))ram[RAM_SIZE] = { 0 };
uint32_t rom[] = {
    0x00000297,                 /* 1:  auipc  t0, %pcrel_hi(fw_dyn) */
    0x02828613,                 /*     addi   a2, t0, %pcrel_lo(1b) */
    0xf1402573,                 /*     csrr   a0, mhartid  */
    0x0202a583,                 /*     lw     a1, 32(t0) */
    0x0182a283,                 /*     lw     t0, 24(t0) */
    0x00028067,                 /*     jr     t0 */
    RAM_BASE  ,                 /* start: .dword */
    0x00000000,
    FDT_ADDR  ,                 /* fdt_laddr: .dword */
    0x00000000,
                                /* fw_dyn: */
    0x4942534f,                 /* OSBI */
    // 0x00000000,
    0x00000002,                 /* Version */
    // 0x00000000,
    LINUX_ADDR,                 /* Next stage addr */
    // 0x00000000,
    0x00000001,                 /* Next stage mode (Supervisor) */
    // 0x00000000,
    0x00000000,                 /* OpenSBI options */
    // 0x00000000,
    0x00000000,                 /* Boot Hart */
    // 0x00000000
};

static bool rom_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    (void) priv;
    if(access == bus_write){
        printf("Writing to ROM, dying uwu\n");
        exit(-1);
    }
    memcpy(val, &((uint8_t*)rom)[addr], len);

    if(len == 4)
        return true;

    uint8_t bits = len << 3;
    *val = extract_bits(*val, (bits-1), (bits-1)) ? *val | sext_bits(bits) : *val & ~sext_bits(bits);
    return true;
}

bool ram_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    (void) priv;
    if(access == bus_write){
        memcpy(&ram[addr], val, len);
        return true;
    }
    memcpy(val, &ram[addr], len);
    
    if(len == 4)
        return true;

    uint8_t bits = len << 3;
    *val = extract_bits(*val, (bits-1), (bits-1)) ? *val | sext_bits(bits) : *val & ~sext_bits(bits);
    return true;
}

bool soc_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    if(ROM_BASE <= addr && addr < ROM_BASE + ROM_SIZE){
        return rom_access_func(addr-ROM_BASE, priv, access, val, len);
    }
    else if(RAM_BASE <= addr && addr < RAM_BASE + RAM_SIZE){
        return ram_access_func(addr-RAM_BASE, priv, access, val, len);
    }
    else if(UART_BASE <= addr && addr < UART_BASE + UART_SIZE){
        // printf("UART access\n");
        // printf("Addr: %016lx\n", addr);
        // printf("Val: %016lx\n", *val);
        // printf("Len : %u\n", len);
        // getchar();
        return uart_bus_access_func(addr-UART_BASE, priv, access, val, len);
    }
    else if(PLIC_BASE <= addr && addr < PLIC_BASE + PLIC_SIZE){
        bool ret_val = plic_bus_access_func(addr-PLIC_BASE, priv, access, val, len);
        // printf("PLIC access\n");
        // printf("Addr: %08x\n", addr);
        // printf("Val: %08x\n", *val);
        // printf("Len : %u\n", len);
        // printf("Access type: %u\n", access);
        // // core_dump();
        // printf("\n");
        // getchar();
        return ret_val;
    }
    else if(CLINT_BASE <= addr && addr < CLINT_BASE + CLINT_SIZE){
        // printf("CLINT access\n");
        // printf("Addr: %016lx\n", addr);
        // printf("Val: %016lx\n", *val);
        // printf("Len : %u\n", len);
        // printf("Access type: %u\n", access);
        // // core_dump();
        // printf("\n");
        // getchar();
        return clint_bus_access_func(addr-CLINT_BASE, priv, access, val, len);
    }
    else if(BLK_BASE <= addr && addr < BLK_BASE + BLK_SIZE){
        bool ret_val = blk_bus_access_func(addr-BLK_BASE, priv, access, val, len);
        // printf("BLK access\n");
        // printf("Addr: %08x\n", addr);
        // printf("Val: %08x\n", *val);
        // printf("Len : %u\n", len);
        // printf("Access type: %u\n", access);
        // core_dump();
        // printf("\n");
        // getchar();
        return ret_val;
    }
    else if(SYSCON_BASE <= addr && addr <= SYSCON_BASE + SYSCON_SIZE){
        printf("SYSCON access\n");
        printf("Addr: %08x\n", addr);
        printf("Val: %08x\n", *val);
        printf("Len : %u\n", len);
        printf("Access type: %u\n", access);
        // core_dump();
        // printf("\n");
        // getchar();
        if(access == bus_write && addr - SYSCON_BASE == 0 && *val == 0x5555){
            printf("SYSCON POWEROFF\n");
            exit(0);
        }
        return true;
    }
    else{
        printf("Addr: %016x\n", addr);
        printf("Invalid physical address, dying uwu\n");
        core_dump();
        exit(-1);
    }
}

static bool mmu_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    priv_level effective_priv;
    trap_cause_exception acccess_fault, page_fault;
    uint32_t satp, xstatus;
    bool sum, mxr;

    uint32_t phy_addr, i, vpn[SV32_LEVELS], ppn, pte, offset;    // [LEVELS]
    
    // Check for MPRV override and machine mode.
    if(priv == machine){
        csr_read(machine, MSTATUS, &xstatus);

        // If MPRV is not set, No translation takes place
        if(!extract_bits(xstatus, MSTATUS_MPRV_BIT, MSTATUS_MPRV_BIT) || access == bus_fetch)
            return soc_bus_access_func(addr, priv, access, val, len);

        // If MPRV is set, set the effective priv to MPP
        effective_priv = (priv_level)extract_bits(xstatus, MSTATUS_MPP_BIT + 1, MSTATUS_MPP_BIT);
    }
    else
        effective_priv = priv;


    csr_read(supervisor, SSTATUS, &xstatus);
    csr_read(supervisor, SATP, &satp);

    // If mode is bare, exit
    if((satp >> SATP_MODE_BIT) != SV32_MODE)
        return soc_bus_access_func(addr, priv, access, val, len);

    // Set the correct faults
    if(access == bus_write){
        acccess_fault = str_amo_access_fault;
        page_fault = str_amo_page_fault;
    }
    else if(access == bus_read){
        acccess_fault = load_access_fault;
        page_fault = load_page_fault;
    }
    else{
        acccess_fault = instr_access_fault;
        page_fault = instr_page_fault;
    }

    // Extracting useful info
    sum = (bool)extract_bits(xstatus, MSTATUS_SUM_BIT, MSTATUS_SUM_BIT);
    mxr = (bool)extract_bits(xstatus, MSTATUS_MXR_BIT, MSTATUS_MXR_BIT);

    ppn = (satp & PPN_MASK) << PAGE_SIZE_SHIFT;
    vpn[1] = (addr >> 22) & VPN_MASK;
    vpn[0] = (addr >> 12) & VPN_MASK;
    offset = addr & OFFSET_MASK;

    // i = LEVELS - 1, *PAGESIZE => << 12
    for(i = SV32_LEVELS - 1; i >= 0; i--){
        // Only if we are unable to access a PTE is it an access fault
        // Otherwise it's always a page fault!!
        if(!soc_bus_access_func(ppn | (vpn[i] << PTE_SIZE_SHIFT), supervisor, bus_read, &pte, 4)){
            core_prepare_sync_trap(acccess_fault, addr);
            return false;
        }

        if(!extract_bits(pte, PTE_V, PTE_V)){
            core_prepare_sync_trap(page_fault, addr);
            return false;
        }

        if(!(pte & PTE_RWX_MASK)){
            ppn = ((pte >> 10) & PPN_MASK) << PAGE_SIZE_SHIFT;
            continue;
        }

        // // Found a leaf page
        // if(i != 0){
        //     printf("Pages bigger than 4KiB not supported, dying uwu\n");
        //     exit(-1);
        // }

        // Necessary translation
        if(i == 1)
            phy_addr = (((pte >> 20) & 0x00000fff) << 22) | (vpn[0] << 12) | offset;
        else if(i == 0)
            phy_addr = (((pte >> 10) & 0x003fffff) << 12) | offset;

        // All the mandatory checks
        if(access == bus_fetch && !extract_bits(pte, PTE_X, PTE_X)){
            core_prepare_sync_trap(page_fault, addr);
            return false;
        }
        else if(access == bus_read && !(extract_bits(pte, PTE_R, PTE_R) || (mxr && extract_bits(pte, PTE_X, PTE_X)))){
            core_prepare_sync_trap(page_fault, addr);
            return false;
        }
        else if(access == bus_write && !extract_bits(pte, PTE_W, PTE_W)){
            core_prepare_sync_trap(page_fault, addr);
            return false;
        }
        
        if(priv == supervisor && extract_bits(pte, PTE_U, PTE_U) && !sum){
            core_prepare_sync_trap(page_fault, addr);
            return false;
        }

        if(priv == user && !extract_bits(pte, PTE_U, PTE_U)){
            core_prepare_sync_trap(page_fault, addr);
            return false;     
        }

        // Setting the access and dirty bits
        if(access == bus_write){
            pte |= 1 << PTE_D;
            soc_bus_access_func(ppn | (vpn[i] << PTE_SIZE_SHIFT), supervisor, bus_write, &pte, 4);
        }
        else{
            pte |= 1 << PTE_A;
            soc_bus_access_func(ppn | (vpn[i] << PTE_SIZE_SHIFT), supervisor, bus_write, &pte, 4);
        }
        // Done with the translation and checks :)
        return soc_bus_access_func(phy_addr, effective_priv, access, val, len);
    }
    // We faulted
    core_prepare_sync_trap(page_fault, addr);
    return false;
}

void memory_init(char* sbi_file, char* linux_file, char* initrd_file, char* dtb_file){
    helper_write_from_file(sbi_file, ram, RAM_SIZE);
    helper_write_from_file(dtb_file, &ram[FDT_ADDR-RAM_BASE], MiB_16);
    helper_write_from_file(linux_file, &ram[LINUX_ADDR - RAM_BASE], RAM_SIZE - (LINUX_ADDR - RAM_BASE));
    helper_write_from_file(initrd_file, &ram[INITRD_ADDR - RAM_BASE], RAM_SIZE - (INITRD_ADDR - RAM_BASE));
    blk_init(initrd_file);
}


bool memory_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    return mmu_bus_access_func(addr, priv, access, val, len);
}

uint8_t* memory_get_ram_ptr(){
    return ram;
}
