#include<stdio.h>
#include<string.h>
#include"helpers.h"
#include"csr.h"
#include"memory.h"

bool memory_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len);

struct inst_properties{
    uint32_t inst;
    uint8_t opcode;
    uint8_t rs1;
    uint8_t rs2;
    uint8_t rd;
    uint8_t funct3;
    uint8_t funct7;
    uint8_t shamt;
    bool bit25;

    uint32_t imm_u;
    uint32_t imm_j;
    uint32_t imm_i;
    uint32_t imm_s;
    uint32_t imm_b;

    bool invalid;
} inst_vals;

struct core{
    priv_level priv;
    uint32_t pc;
    uint32_t next_pc;
    uint32_t regs[32];

    bool debug[128];
    uint64_t cycle;

    bool sync_trap_pending;
    uint32_t sync_trap_cause;
    uint32_t sync_tval;
} core;

// ---------------------------------- I -------------------------------------------------------------------------

static inline void lui(){
    core.debug[0] = true;
    core.regs[inst_vals.rd] = inst_vals.imm_u;
    core.next_pc = core.pc + 4;
}

static inline void auipc(){
    core.debug[1] = true;
    core.regs[inst_vals.rd] = core.pc + inst_vals.imm_u;
    core.next_pc = core.pc + 4;
}

static inline void addi(){
    core.debug[2] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] + inst_vals.imm_i;
    core.next_pc = core.pc + 4;
}

static inline void slti(){
    core.debug[3] = true;
    if((int32_t)core.regs[inst_vals.rs1] < (int32_t)inst_vals.imm_i)
        core.regs[inst_vals.rd] = 1;
    else
        core.regs[inst_vals.rd] = 0;
    core.next_pc = core.pc + 4;
}

static inline void sltiu(){
    core.debug[4] = true;
    if((uint32_t)core.regs[inst_vals.rs1] < (uint32_t)inst_vals.imm_i)
        core.regs[inst_vals.rd] = 1;
    else
        core.regs[inst_vals.rd] = 0;
    core.next_pc = core.pc + 4;
}

static inline void xori(){
    core.debug[5] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] ^ inst_vals.imm_i;
    core.next_pc = core.pc + 4;
}

static inline void ori(){
    core.debug[6] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] | inst_vals.imm_i;
    core.next_pc = core.pc + 4;
}

static inline void andi(){
    core.debug[7] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] & inst_vals.imm_i;
    core.next_pc = core.pc + 4;
}

static inline void slli(){
    core.debug[8] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] << inst_vals.shamt;
    core.next_pc = core.pc + 4;
}

static inline void srli_srai(){
    core.debug[9] = true;
    bool bit30 = (bool)extract_bits(inst_vals.inst, 30, 30);
    bool bit31 = (bool)extract_bits(core.regs[inst_vals.rs1], 31, 31);
    
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] >> inst_vals.shamt;

    if(inst_vals.shamt == 0){
        core.next_pc = core.pc + 4;
        return;
    }

    core.regs[inst_vals.rd] |= bit31 && bit30 ? sext_bits(32 - inst_vals.shamt) : 0;
    core.next_pc = core.pc + 4;
}

static inline void add_sub(){
    core.debug[10] = true;
    bool bit30 = (bool)extract_bits(inst_vals.inst, 30, 30);
    if(bit30)
        core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] - core.regs[inst_vals.rs2];
    else
        core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] + core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void sll(){
    core.debug[11] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] << (core.regs[inst_vals.rs2] & 0x1f);
    core.next_pc = core.pc + 4;
}

static inline void slt(){
    core.debug[12] = true;
    if((int32_t)core.regs[inst_vals.rs1] < (int32_t)core.regs[inst_vals.rs2])
        core.regs[inst_vals.rd] = 1;
    else
        core.regs[inst_vals.rd] = 0;
    core.next_pc = core.pc + 4;
}

static inline void sltu(){
    core.debug[13] = true;
    if((uint32_t)core.regs[inst_vals.rs1] < (uint32_t)core.regs[inst_vals.rs2])
        core.regs[inst_vals.rd] = 1;
    else
        core.regs[inst_vals.rd] = 0;
    core.next_pc = core.pc + 4;
}

static inline void xor(){
    core.debug[14] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] ^ core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void srl_sra(){
    core.debug[15] = true;
    bool bit30 = (bool)extract_bits(inst_vals.inst, 30, 30);
    bool bit31 = (bool)extract_bits(core.regs[inst_vals.rs1], 31, 31);
    uint8_t shamt = core.regs[inst_vals.rs2] & 0x1f;
    
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] >> shamt;
    
    if(shamt == 0){
        core.next_pc = core.pc + 4;
        return;
    }
    
    core.regs[inst_vals.rd] |= bit31 && bit30 ? sext_bits(32 - shamt) : 0;
    core.next_pc = core.pc + 4;
}

static inline void or(){
    core.debug[16] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] | core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void and(){
    core.debug[17] = true;
    core.regs[inst_vals.rd] = core.regs[inst_vals.rs1] & core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void jal(){
    core.debug[18] = true;
    core.regs[inst_vals.rd] = core.pc + 4;
    core.next_pc = (core.pc + inst_vals.imm_j) & sext_bits(2);
}

static inline void jalr(){
    core.debug[19] = true;
    uint32_t temp = core.pc + 4;
    core.next_pc = (core.regs[inst_vals.rs1] + inst_vals.imm_i) & sext_bits(2);
    core.regs[inst_vals.rd] = temp;
}

static inline void beq(){
    core.debug[20] = true;
    if(core.regs[inst_vals.rs1] == core.regs[inst_vals.rs2])
        core.next_pc = (core.pc + inst_vals.imm_b) & sext_bits(2);
    else
        core.next_pc = core.pc + 4;
}

static inline void bne(){
    core.debug[21] = true;
    if(core.regs[inst_vals.rs1] != core.regs[inst_vals.rs2])
        core.next_pc = (core.pc + inst_vals.imm_b) & sext_bits(2);
    else
        core.next_pc = core.pc + 4;
}

static inline void blt(){
    core.debug[22] = true;
    if((int32_t)core.regs[inst_vals.rs1] < (int32_t)core.regs[inst_vals.rs2])
        core.next_pc = (core.pc + inst_vals.imm_b) & sext_bits(2);
    else
        core.next_pc = core.pc + 4;
}

static inline void bge(){
    core.debug[23] = true;
    if((int32_t)core.regs[inst_vals.rs1] >= (int32_t)core.regs[inst_vals.rs2])
        core.next_pc = (core.pc + inst_vals.imm_b) & sext_bits(2);
    else
        core.next_pc = core.pc + 4;
}

static inline void bltu(){
    core.debug[24] = true;
    if((uint32_t)core.regs[inst_vals.rs1] < (uint32_t)core.regs[inst_vals.rs2])
        core.next_pc = (core.pc + inst_vals.imm_b) & sext_bits(2);
    else
        core.next_pc = core.pc + 4;
}

static inline void bgeu(){
    core.debug[25] = true;
    if((uint32_t)core.regs[inst_vals.rs1] >= (uint32_t)core.regs[inst_vals.rs2])
        core.next_pc = (core.pc + inst_vals.imm_b) & sext_bits(2);
    else
        core.next_pc = core.pc + 4;
}

static inline void lb(){
    core.debug[32] = true;
    uint32_t val;
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_i;
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val, 1))
        return;
    core.regs[inst_vals.rd] = val;
    core.next_pc = core.pc + 4;
}

static inline void lh(){
    core.debug[33] = true;
    uint32_t val;
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_i;
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val, 2))
        return;
    core.regs[inst_vals.rd] = val;
    core.next_pc = core.pc + 4;
}

static inline void lw(){
    core.debug[34] = true;
    uint32_t val;
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_i;
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val, 4))
        return;
    core.regs[inst_vals.rd] = val;
    core.next_pc = core.pc + 4;
}

static inline void lbu(){
    core.debug[36] = true;
    uint32_t val;
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_i;
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val, 1))
        return;
    core.regs[inst_vals.rd] = val & 0xff;
    core.next_pc = core.pc + 4;
}

static inline void lhu(){
    core.debug[37] = true;
    uint32_t val;
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_i;
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val, 2))
        return;
    core.regs[inst_vals.rd] = val & 0xffff;
    core.next_pc = core.pc + 4;
}

static inline void sb(){
    core.debug[39] = true;
    uint32_t val = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_s;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val, 1))
        return;
    core.next_pc = core.pc + 4;
}

static inline void sh(){
    core.debug[40] = true;
    uint32_t val = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_s;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val, 2))
        return;
    core.next_pc = core.pc + 4;
}

static inline void sw(){
    core.debug[41] = true;
    uint32_t val = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1] + inst_vals.imm_s;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val, 4))
        return;
    core.next_pc = core.pc + 4;
}

// ---------------------------------- Env -------------------------------------------------------------------------

static inline void fence_fencei(){
    core.debug[43] = true;
    core.next_pc = core.pc + 4;
}

static inline void ecall_ebreak(){
    core.debug[44] = true;
    if(inst_vals.rs2 == 1){
        core_prepare_sync_trap(breakpoint, core.pc);
        return;
    }
    else if(inst_vals.rs2 == 0){
        if(core.priv == machine)
            core_prepare_sync_trap(machine_ecall, 0);
        else if(core.priv == supervisor)
            core_prepare_sync_trap(supervisor_ecall, 0);
        else
            core_prepare_sync_trap(user_ecall, 0);
    }
    else
        inst_vals.invalid = true;
    return;
}

static inline void sret_wfi(){
    core.debug[45] = true;
    uint32_t pie, xstatus, addr;

    if(inst_vals.rs2 == 5){
        // inst_vals.invalid = true;
        core.next_pc = core.pc + 4;
        return;
    }
    
    if(core.priv != supervisor){
        inst_vals.invalid = true;
        return;
    }

    csr_read(machine, MSTATUS, &xstatus);
    pie = extract_bits(xstatus, MSTATUS_SPIE_BIT, MSTATUS_SPIE_BIT);
    xstatus &= ~(1 << MSTATUS_SPIE_BIT);
    xstatus &= ~(1 << MSTATUS_SIE_BIT);
    xstatus |=  (pie << MSTATUS_SIE_BIT);
    xstatus &= ~(1 << MSTATUS_MPRV_BIT);
    csr_write(machine, MSTATUS, xstatus);

    core.priv = (priv_level)extract_bits(xstatus, MSTATUS_SPP_BIT, MSTATUS_SPP_BIT);
    csr_read(machine, SEPC, &addr);
    core.next_pc = addr;

    // printf("TAKING SRET TRAP WOHOOOOOOOOOO\n");
    // printf("%lu\n", core.priv);
    // printf("0x%016lx\n", core.pc);
    // printf("0x%016lx\n\n", addr);
    // getchar();
    return;
}

static inline void mret(){
    core.debug[46] = true;
    uint32_t pie, xstatus, addr;

    if(core.priv != machine){
        inst_vals.invalid = true;
        return;
    }

    csr_read(machine, MSTATUS, &xstatus);
    pie = extract_bits(xstatus, MSTATUS_MPIE_BIT, MSTATUS_MPIE_BIT);
    xstatus &= ~(1 << MSTATUS_MPIE_BIT);
    xstatus &= ~(1 << MSTATUS_MIE_BIT);
    xstatus |= (pie << MSTATUS_MIE_BIT);
    csr_write(machine, MSTATUS, xstatus);

    core.priv = (priv_level)extract_bits(xstatus, MSTATUS_MPP_BIT + 1, MSTATUS_MPP_BIT);
    csr_read(machine, MEPC, &addr);
    core.next_pc = addr;

    // printf("TAKING MRET TRAP WOHOOOOOOOOOO\n");
    // printf("%lu\n", core.priv);
    // printf("0x%016lx\n", core.pc);
    // printf("0x%016lx\n\n", addr);
    // getchar();
    return;
}

static inline void sfence_vma(){
    core.debug[47] = true;
    core.next_pc = core.pc + 4;
    // inst_vals.invalid = true;
}

static inline void sinval_vma(){
    core.debug[48] = true;
    inst_vals.invalid = true;
}

static inline void sfence_w_inval_inval_ir(){
    core.debug[49] = true;
    inst_vals.invalid = true; 
}

static inline void csrrw(){
    core.debug[50] = true;
    uint32_t val;
    uint16_t csr_addr = (inst_vals.funct7 << 5) | inst_vals.rs2;
    
    if(inst_vals.rd == 0){
        if(!csr_write(core.priv, csr_addr, core.regs[inst_vals.rs1])){
            inst_vals.invalid = true;
            return;
        }
    }
    else{
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        if(!csr_write(core.priv, csr_addr, core.regs[inst_vals.rs1])){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    core.next_pc = core.pc + 4;
}

static inline void csrrs(){
    core.debug[51] = true;
    uint32_t val;
    uint16_t csr_addr = (inst_vals.funct7 << 5) | inst_vals.rs2;
    
    if(inst_vals.rs1 == 0){
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    else{
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        if(!csr_write(core.priv, csr_addr, core.regs[inst_vals.rs1] | val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    core.next_pc = core.pc + 4;
}

static inline void csrrc(){
    core.debug[52] = true;
    uint32_t val;
    uint16_t csr_addr = (inst_vals.funct7 << 5) | inst_vals.rs2;
    
    if(inst_vals.rs1 == 0){
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    else{
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        if(!csr_write(core.priv, csr_addr, ~core.regs[inst_vals.rs1] & val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    core.next_pc = core.pc + 4;
}

static inline void csrrwi(){
    core.debug[53] = true;
    uint32_t val;
    uint32_t imm = inst_vals.rs1;
    uint16_t csr_addr = (inst_vals.funct7 << 5) | inst_vals.rs2;
    
    if(imm == 0){
        if(!csr_write(core.priv, csr_addr, core.regs[inst_vals.rs1])){
            inst_vals.invalid = true;
            return;
        }
    }
    else{
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        if(!csr_write(core.priv, csr_addr, imm)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    core.next_pc = core.pc + 4;
}

static inline void csrrsi(){
    core.debug[54] = true;
    uint32_t val;
    uint32_t imm = inst_vals.rs1;
    uint16_t csr_addr = (inst_vals.funct7 << 5) | inst_vals.rs2;
    
    if(imm == 0){
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    else{
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        if(!csr_write(core.priv, csr_addr, imm | val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    core.next_pc = core.pc + 4; 
}

static inline void csrrci(){
    core.debug[55] = true;
    uint32_t val;
    uint32_t imm = inst_vals.rs1;
    uint16_t csr_addr = (inst_vals.funct7 << 5) | inst_vals.rs2;
    
    if(imm == 0){
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    else{
        if(!csr_read(core.priv, csr_addr, &val)){
            inst_vals.invalid = true;
            return;
        }
        if(!csr_write(core.priv, csr_addr, ~imm & val)){
            inst_vals.invalid = true;
            return;
        }
        core.regs[inst_vals.rd] = val;
    }
    core.next_pc = core.pc + 4;
}


// ---------------------------------- M -------------------------------------------------------------------------

static inline void mul(){
    core.debug[56] = true;
    core.regs[inst_vals.rd] = (int32_t)core.regs[inst_vals.rs1] * (int32_t)core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void mulh(){
    core.debug[57] = true;
    int64_t temp1 = (int64_t)(int32_t)core.regs[inst_vals.rs1];
    int64_t temp2 = (int64_t)(int32_t)core.regs[inst_vals.rs2];
    core.regs[inst_vals.rd] = (temp1 * temp2) >> 32;
    core.next_pc = core.pc + 4;
}

static inline void mulhsu(){
    core.debug[58] = true;
    uint64_t temp1 = (int64_t)(int32_t)core.regs[inst_vals.rs1];
    uint64_t temp2 = (uint64_t)core.regs[inst_vals.rs2];
    core.regs[inst_vals.rd] = (temp1 * temp2) >> 32;
    core.next_pc = core.pc + 4;
}

static inline void mulhu(){
    core.debug[59] = true;
    uint64_t temp = (uint64_t)core.regs[inst_vals.rs1] * (uint64_t)core.regs[inst_vals.rs2];
    core.regs[inst_vals.rd] = temp >> 32;
    core.next_pc = core.pc + 4;
}

static inline void divv(){
    core.debug[60] = true;
    if(core.regs[inst_vals.rs2] == 0)
        core.regs[inst_vals.rd] = 0xffffffff;
    else if(core.regs[inst_vals.rs2] == 0xffffffff &&
            core.regs[inst_vals.rs1] == 0x80000000){
        core.regs[inst_vals.rd] = 0x80000000;
    }
    else
        core.regs[inst_vals.rd] = (int32_t)core.regs[inst_vals.rs1] / (int32_t)core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void divu(){
    core.debug[61] = true;
    if(core.regs[inst_vals.rs2] == 0)
        core.regs[inst_vals.rd] = 0xffffffff;
    else
        core.regs[inst_vals.rd] = (uint32_t)core.regs[inst_vals.rs1] / (uint32_t)core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void rem(){
    core.debug[62] = true;
    if(core.regs[inst_vals.rs2] == 0)
        core.regs[inst_vals.rd] = core.regs[inst_vals.rs1];
    else if(core.regs[inst_vals.rs2] == 0xffffffff && core.regs[inst_vals.rs1] == 0x80000000)
        core.regs[inst_vals.rd] = 0;
    else
        core.regs[inst_vals.rd] = (int32_t)core.regs[inst_vals.rs1] % (int32_t)core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

static inline void remu(){
    core.debug[63] = true;
    if(core.regs[inst_vals.rs2] == 0)
        core.regs[inst_vals.rd] = core.regs[inst_vals.rs1];
    else
        core.regs[inst_vals.rd] = (uint32_t)core.regs[inst_vals.rs1] % (uint32_t)core.regs[inst_vals.rs2];
    core.next_pc = core.pc + 4;
}

// ---------------------------------- A -------------------------------------------------------------------------

static inline void lr(){
    core.debug[69] = true;
    uint32_t val;
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val, 4))
        return;
    core.regs[inst_vals.rd] = val;
    core.next_pc = core.pc + 4;
}

static inline void sc(){
    core.debug[70] = true;
    uint32_t val = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val, 4))
        return;
    core.regs[inst_vals.rd] = 0;
    core.next_pc = core.pc + 4;
}

static inline void amoswap(){
    core.debug[71] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amoadd(){
    core.debug[72] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    val2 = val1 + val2;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amoxor(){
    core.debug[73] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    val2 = val1 ^ val2;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amoand(){
    core.debug[74] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    val2 = val1 & val2;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amoor(){
    core.debug[75] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    val2 = val1 | val2;
    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amomin(){
    core.debug[76] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    
    val2 = (int32_t)val1 > (int32_t)val2 ? val2 : val1;

    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amomax(){
    core.debug[77] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    
    val2 = (int32_t)val1 > (int32_t)val2 ? val1 : val2;

    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amominu(){
    core.debug[78] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    
    val2 = (uint32_t)val1 > (uint32_t)val2 ? val2 : val1;

    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

static inline void amomaxu(){
    core.debug[79] = true;
    uint32_t val1;
    uint32_t val2 = core.regs[inst_vals.rs2];
    uint32_t addr = core.regs[inst_vals.rs1];
    if(!memory_bus_access_func(addr, core.priv, bus_read, &val1, 4))
        return;
    
    val2 = (uint32_t)val1 > (uint32_t)val2 ? val1 : val2;

    if(!memory_bus_access_func(addr, core.priv, bus_write, &val2, 4))
        return;
    core.regs[inst_vals.rd] = val1;
    core.next_pc = core.pc + 4;
}

// ---------------------------------- END -------------------------------------------------------------------------

void core_init(){
    memset(&core, 0, sizeof(core));
    core.priv = machine;
    core.next_pc = core.pc = ROM_BASE;
    core.sync_trap_pending = false;

    core.cycle = 0;
    // core_dump();
}

void core_dump(){
    uint32_t val1, val2;
    printf("Cycle: %lu\n", core.cycle);
    printf("Inst: 0x%08x\tInvalid: %s\n", inst_vals.inst, inst_vals.invalid ? "true" : "false");
    printf("PC: 0x%08x\t\tNPC:0x%08x\n\n", core.pc, core.next_pc);
    for(int i = 0; i < 16; i++)
        printf("x%02d:0x%08x\t\tx%02d:0x%08x\n", i, core.regs[i], i+16, core.regs[i+16]);
    
    csr_read(machine, MIE, &val1);
    csr_read(machine, MIP, &val2);
    printf("MIE:\t0x%08x\t\tMIP:\t0x%08x\n", val1, val2);

    csr_read(machine, MIDELEG, &val1);
    csr_read(machine, MEDELEG, &val2);
    printf("MIDELEG:0x%08x\t\tMEDELEG:0x%08x\n", val1, val2);

    csr_read(machine, MTVEC, &val1);
    csr_read(machine, MEPC, &val2);
    printf("MTVEC:\t0x%08x\t\tMEPC:\t0x%08x\n", val1, val2);

    csr_read(machine, MCAUSE, &val1);
    csr_read(machine, MTVAL, &val2);
    printf("MCAUSE:\t0x%08x\t\tMTVAL:\t0x%08x\n", val1, val2);

    csr_read(machine, SEPC, &val1);
    csr_read(machine, SCAUSE, &val2);
    printf("SEPC:\t0x%08x\t\tSCAUSE:\t0x%08x\n", val1, val2);

    csr_read(machine, STVEC, &val1);
    csr_read(machine, STVAL, &val2);
    printf("STVEC:\t0x%08x\t\tSTVAL:\t0x%08x\n", val1, val2);

    csr_read(machine, MSTATUS, &val1);
    csr_read(machine, SSTATUS, &val2);
    printf("MSTATUS:0x%08x\t\tSSTATUS:0x%08x\n", val1, val2);

    printf("Sync Pending:%d, Cause:%d, TVAL:%d\n", core.sync_trap_pending, core.sync_trap_cause, core.sync_tval);
}

bool core_fetch(uint32_t* inst){
    // if(core.regs[10] == 0x00092e00){
    // if(core.next_pc == 0x1f3d4){
    // // if(core.priv == user){
    //     temp();
    //     // core_dump();
    //     // exit(-1);
    // }
    uint32_t val;
    core.pc = core.next_pc;
    if(memory_bus_access_func(core.next_pc, core.priv, bus_fetch, &val, 4)){
        *inst = (uint32_t)val;
        return true;
    }
    *inst = 0;
    return false;
}

void core_decode(uint32_t inst){
    // Assign
    inst_vals.inst     = inst;
    inst_vals.opcode   = extract_bits(inst,  6,  0);
    inst_vals.rs1      = extract_bits(inst, 19, 15);
    inst_vals.rs2      = extract_bits(inst, 24, 20);
    inst_vals.rd       = extract_bits(inst, 11,  7);
    inst_vals.funct3   = extract_bits(inst, 14, 12);
    inst_vals.funct7   = extract_bits(inst, 31, 25);
    inst_vals.shamt    = extract_bits(inst, 25, 20);

    inst_vals.bit25     = (bool)extract_bits(inst, 25, 25);

    inst_vals.imm_u    = (extract_bits(inst, 31, 12) << 12);
    inst_vals.imm_j    = (extract_bits(inst, 31, 31) << 20) |
                         (extract_bits(inst, 30, 21) <<  1) |
                         (extract_bits(inst, 20, 20) << 11) |
                         (extract_bits(inst, 19, 12) << 12);
    inst_vals.imm_i    = (extract_bits(inst, 31, 20) <<  0);
    inst_vals.imm_s    = (extract_bits(inst, 31, 25) <<  5) |
                         (extract_bits(inst, 11,  7) <<  0);
    inst_vals.imm_b    = (extract_bits(inst, 31, 31) << 12) |
                         (extract_bits(inst, 30, 25) <<  5) |
                         (extract_bits(inst, 11,  8) <<  1) |
                         (extract_bits(inst,  7,  7) << 11);
    // SExt
    if(extract_bits(inst_vals.imm_j, 20, 20))
        inst_vals.imm_j |= sext_bits(21);

    if(extract_bits(inst_vals.imm_i, 11, 11))
        inst_vals.imm_i |= sext_bits(12);
    
    if(extract_bits(inst_vals.imm_s, 11, 11))
        inst_vals.imm_s |= sext_bits(12);

    if(extract_bits(inst_vals.imm_b, 12, 12))
        inst_vals.imm_b |= sext_bits(13);
    
    inst_vals.invalid = false;
}

void core_execute(){
    core.regs[0] = 0;
    switch(inst_vals.opcode){
        case 0b0110111:     // LUI
            lui();
            break;
        case 0b0010111:     // AUIPC
            auipc();
            break;
        case 0b1100011:     // Branches
            switch(inst_vals.funct3){
                case 0b000:         // BEQ
                    beq();
                    break;
                case 0b001:         // BNE
                    bne();
                    break;
                case 0b100:         // BLT
                    blt();
                    break;
                case 0b101:         // BGE
                    bge();
                    break;
                case 0b110:         // BLTU
                    bltu();
                    break;
                case 0b111:         // BGEU
                    bgeu();
                    break;
                default:
                    inst_vals.invalid = true;
                    break;
            }
            break;
        case 0b0000011:     // Load bus_fetchs
            switch(inst_vals.funct3){
                case 0b000:
                    lb();
                    break;
                case 0b001:
                    lh();
                    break;
                case 0b010:
                    lw();
                    break;
                case 0b100:
                    lbu();
                    break;
                case 0b101:
                    lhu();
                    break;
                default:
                    inst_vals.invalid = true;
            }
            break;
        case 0b0100011:     // Store bus_fetchs
            switch(inst_vals.funct3){
                case 0b000:
                    sb();
                    break;
                case 0b001:
                    sh();
                    break;
                case 0b010:
                    sw();
                    break;
                default:
                    inst_vals.invalid = true;
            }
            break;
        case 0b0010011:     // RV32I IMMEDIATES
            switch(inst_vals.funct3){
                case 0b000:     // ADDI
                    addi();
                    break;
                case 0b001:     // SLLI
                    slli();
                    break;
                case 0b010:     // SLTI
                    slti();
                    break;
                case 0b011:     // SLTIU
                    sltiu();
                    break;
                case 0b100:     // XORI
                    xori();
                    break;
                case 0b101:     // SRLI SRAI
                    srli_srai();
                    break;
                case 0b110:     // ORI
                    ori();
                    break;
                case 0b111:     // ANDI
                    andi();
                    break;
                default:
                    inst_vals.invalid = true;
                    break;
            }
            break;
        case 0b0110011:     // RV64 2 regs
            switch((int)inst_vals.bit25){
                case false:
                    switch(inst_vals.funct3){
                        case 0b000:     // ADD SUB
                            add_sub();
                            break;
                        case 0b001:     // SLL
                            sll();
                            break;
                        case 0b010:     // SLT
                            slt();
                            break;
                        case 0b011:     // SLTU
                            sltu();
                            break;
                        case 0b100:     // XOR
                            xor();
                            break;
                        case 0b101:     // SRL SRA
                            srl_sra();
                            break;
                        case 0b110:     // OR
                            or();
                            break;
                        case 0b111:     // AND
                            and();
                            break;
                        default:
                            inst_vals.invalid = true;
                            break;
                    }
                    break;
                case true:
                    switch(inst_vals.funct3){
                        case 0b000:     // MUL
                            mul();
                            break;
                        case 0b001:     // MULH
                            mulh();
                            break;
                        case 0b010:     // MULHSU
                            mulhsu();
                            break;
                        case 0b011:     // MULHU
                            mulhu();
                            break;
                        case 0b100:     // DIV
                            divv();
                            break;
                        case 0b101:     // DIVU
                            divu();
                            break;
                        case 0b110:     // REM
                            rem();
                            break;
                        case 0b111:     // REMU
                            remu();
                            break;
                        default:
                            inst_vals.invalid = true;
                            break;
                    }
                    break;
                default:
                    inst_vals.invalid = true;
                    break;                  
            }
            break;
        case 0b0001111:     // FENCE FENCE.I
            fence_fencei();
            break;
        case 0b1110011:
            switch(inst_vals.funct3){
                case 0b000:
                    switch(inst_vals.funct7){
                        case 0b0000000:
                            ecall_ebreak();
                            break;
                        case 0b0001000:
                            sret_wfi();
                            break;
                        case 0b0011000:
                            mret();
                            break;
                        case 0b0001001:
                            sfence_vma();
                            break;
                        case 0b0001011:
                            sinval_vma();
                            break;
                        case 0b0001100:
                            sfence_w_inval_inval_ir();
                            break;
                        default:
                            inst_vals.invalid = true;
                    }
                    break;
                case 0b001:
                    csrrw();
                    break;
                case 0b010:
                    csrrs();
                    break;
                case 0b011:
                    csrrc();
                    break;
                case 0b101:
                    csrrwi();
                    break;
                case 0b110:
                    csrrsi();
                    break;
                case 0b111:
                    csrrci();
                    break;
                default:
                    inst_vals.invalid = true;
                    break;
            }
            break;
        case 0b1101111:     // JAL
            jal();
            break;
        case 0b1100111:     // JALR
            jalr();
            break;
        case 0b0101111:         // ATOMICS
            switch(inst_vals.funct7 & ~0x00000003){
                case 0b0001000:
                    lr();
                    break;
                case 0b0001100:
                    sc();
                    break;
                case 0b0000100:
                    amoswap();
                    break;                        
                case 0b0000000:
                    amoadd();
                    break;
                case 0b0010000:
                    amoxor();
                    break;
                case 0b0110000:
                    amoand();
                    break;
                case 0b0100000:
                    amoor();
                    break;
                case 0b1000000:
                    amomin();
                    break;
                case 0b1010000:
                    amomax();
                    break;
                case 0b1100000:
                    amominu();
                    break;
                case 0b1110000:
                    amomaxu();
                    break;
                default:
                    inst_vals.invalid = true;
            }
            break;
        default:
            inst_vals.invalid = true;
            break;
    }
    core.regs[0] = 0;
    if(inst_vals.invalid)
        core_prepare_sync_trap(illegal_instr, 0);
    
    core.cycle++;
}

void core_process_interrupts(bool mei, bool sei, bool mti, bool msi){
    uint32_t ie, xstatus, addr, medeleg;
    bool delegated = false;

    uint32_t mstatus, mideleg, mip, mie;
    bool mstatus_mie, mstatus_sie, mie_val, mip_val, int_val, mideleg_val;
    trap_cause_interrupt interrupt_cause;

    if(msi)
        csr_set_mip(MSIP_BIT);
    else
        csr_clear_mip(MSIP_BIT);
    if(mti)
        csr_set_mip(MTIP_BIT);
    else
        csr_clear_mip(MTIP_BIT);
    if(mei){
        // csr_set_mip(SEIP_BIT);
        csr_set_mip(MEIP_BIT);
    }
    else{
        // csr_clear_mip(SEIP_BIT);
        csr_clear_mip(MEIP_BIT);
    }
    if(sei)
        csr_set_mip(SEIP_BIT);
    else
        csr_clear_mip(SEIP_BIT);

    
    if(core.sync_trap_pending == true){
        if(core.priv != machine){
            csr_read(machine, MEDELEG, &medeleg);
            delegated = (bool)(medeleg & (1 << core.sync_trap_cause));
        }

        if(delegated){
            // xEPC xTVAL and xCAUSE
            csr_write(supervisor, SEPC, core.pc);
            csr_write(supervisor, SCAUSE, core.sync_trap_cause);
            csr_write(supervisor, STVAL, core.sync_tval);

            // xPP & xIE Stack
            csr_read(supervisor, SSTATUS, &xstatus);
            xstatus &= ~(1 << MSTATUS_SPP_BIT);
            xstatus |= (uint32_t)(core.priv & 0x1) << MSTATUS_SPP_BIT;
            ie = extract_bits(xstatus, MSTATUS_SIE_BIT, MSTATUS_SIE_BIT);
            xstatus &= ~(1 << MSTATUS_SIE_BIT);
            xstatus &= ~(1 << MSTATUS_SPIE_BIT);
            xstatus |= ie << MSTATUS_SPIE_BIT;
            csr_write(supervisor, SSTATUS, xstatus);

            // Boilerplate
            csr_read(supervisor, STVEC, &addr);
            core.next_pc = addr;
            core.priv = supervisor;

            // if(core.pc != 0xc0009410){
            //     printf("TAKING SYNC TRAP to S WOHOOOOOOOOOO\n");
            //     core_dump();
            //     printf("Cause: %u\n", core.sync_trap_cause);
            //     printf("Priv: %u\n", core.priv);
            //     printf("0x%08x\n", core.pc);
            //     printf("0x%08x\n\n", addr);
            //     // getchar();
            // }
            core.sync_trap_pending = false;
            core.sync_trap_cause = 0;
            core.sync_tval = 0;
            return;
        }
        else{
            // xEPC xTVAL and xCAUSE
            csr_write(machine, MEPC, core.pc);
            csr_write(machine, MCAUSE, core.sync_trap_cause);
            csr_write(machine, MTVAL, core.sync_tval);

            // xPP & xIE Stack
            csr_read(machine, MSTATUS, &xstatus);
            xstatus &= ~(3 << MSTATUS_MPP_BIT);
            xstatus |= (uint32_t)(core.priv & 0x3) << MSTATUS_MPP_BIT;
            ie = extract_bits(xstatus, MSTATUS_MIE_BIT, MSTATUS_MIE_BIT);
            xstatus &= ~(1 << MSTATUS_MIE_BIT);
            xstatus &= ~(1 << MSTATUS_MPIE_BIT);
            xstatus |= ie << MSTATUS_MPIE_BIT;
            csr_write(machine, MSTATUS, xstatus);

            // Boilerplate
            csr_read(machine, MTVEC, &addr);
            core.next_pc = addr;
            core.priv = machine;

            if(core.pc != 0xc0009410){
                // printf("TAKING SYNC TRAP to M WOHOOOOOOOOOO\n");
                // core_dump();
                // printf("Cause: %u\n", core.sync_trap_cause);
                // printf("Priv: %u\n", core.priv);
                // printf("0x%08x\n", core.pc);
                // printf("0x%08x\n\n", addr);
                // getchar();
            }
            core.sync_trap_pending = false;
            core.sync_trap_cause = 0;
            core.sync_tval = 0;
            return;
        }
    }
    csr_read(machine, MSTATUS, &mstatus);
    csr_read(machine, MIDELEG, &mideleg);
    csr_read(machine, MIP, &mip);
    csr_read(machine, MIE, &mie);
    
    mstatus_mie = (bool)extract_bits(mstatus, MSTATUS_MIE_BIT, MSTATUS_MIE_BIT);
    mstatus_sie = (bool)extract_bits(mstatus, MSTATUS_SIE_BIT, MSTATUS_SIE_BIT);

    for(interrupt_cause = machine_exti;interrupt_cause >= supervisor_swi; interrupt_cause--){
        mip_val = (bool)extract_bits(mip, interrupt_cause, interrupt_cause);
        mie_val = (bool)extract_bits(mie, interrupt_cause, interrupt_cause);
        int_val = mip_val && mie_val;
        mideleg_val = (bool)extract_bits(mideleg, interrupt_cause, interrupt_cause);

        // if(interrupt_cause != machine_ti)
        //     printf("NIGGER\n");

        if(!int_val)
            continue;
        
        
        if(!mideleg_val){
            if((core.priv == machine && !mstatus_mie) || core.priv > machine)
                continue;

            csr_write(machine, MCAUSE, (uint32_t)interrupt_cause | (1 << 31));
            csr_write(machine, MEPC, core.next_pc);
            csr_write(machine, MTVAL, 0);

            // xPP & xIE Stack
            csr_read(machine, MSTATUS, &xstatus);
            xstatus &= ~(3 << MSTATUS_MPP_BIT);
            xstatus |= (uint32_t)(core.priv & 0x3) << MSTATUS_MPP_BIT;
            ie = extract_bits(xstatus, MSTATUS_MIE_BIT, MSTATUS_MIE_BIT);
            xstatus &= ~(1 << MSTATUS_MIE_BIT);
            xstatus &= ~(1 << MSTATUS_MPIE_BIT);
            xstatus |= ie << MSTATUS_MPIE_BIT;
            csr_write(machine, MSTATUS, xstatus);

            // Boilerplate
            csr_read(machine, MTVEC, &addr);
            core.next_pc = addr;
            core.priv = machine;

            if(interrupt_cause == machine_exti || interrupt_cause == supervisor_exti){
                printf("TAKING M Interrupt to M WOHOOOOOOOOOO\n");
                // core_dump();
                printf("%u\n", core.priv);
                printf("0x%08x\n", core.pc);
                printf("0x%08x\n", addr);
                printf("Cause:%u\n\n", interrupt_cause);
                fflush(stdout);
                // getchar();
            }
            return;
        }
        else{
            if((core.priv == supervisor && !mstatus_sie) || core.priv > supervisor)
                continue;

            csr_write(machine, SCAUSE, (uint32_t)interrupt_cause | (1 << 31));
            csr_write(machine, SEPC, core.next_pc);
            csr_write(machine, STVAL, 0);

            // xPP & xIE Stack
            csr_read(machine, SSTATUS, &xstatus);
            xstatus &= ~(1 << MSTATUS_SPP_BIT);
            xstatus |= (uint32_t)(core.priv & 0x1) << MSTATUS_SPP_BIT;
            ie = extract_bits(xstatus, MSTATUS_SIE_BIT, MSTATUS_SIE_BIT);
            xstatus &= ~(1 << MSTATUS_SIE_BIT);
            xstatus &= ~(1 << MSTATUS_SPIE_BIT);
            xstatus |= ie << MSTATUS_SPIE_BIT;
            csr_write(machine, SSTATUS, xstatus);

            // Boilerplate
            csr_read(machine, STVEC, &addr);
            core.next_pc = addr;
            core.priv = supervisor;

            if(interrupt_cause == machine_exti){
                printf("TAKING S Interrupt to S WOHOOOOOOOOOO\n");
                // core_dump();
                printf("%u\n", core.priv);
                printf("0x%08x\n", core.pc);
                printf("0x%08x\n", addr);
                printf("Cause:%u\n\n", interrupt_cause);
                fflush(stdout);
                // getchar();
            }
            return;
        }
        
    }
}

void core_prepare_sync_trap(uint32_t cause, uint32_t tval){
    if(!core.sync_trap_pending){
        core.sync_trap_pending = true;
        core.sync_trap_cause = cause;
        core.sync_tval = tval;
    }
}

void core_memory_dump(uint32_t start_addr, uint32_t end_addr, bool print_addr){
    uint32_t val;

    while(start_addr < end_addr){
        memory_bus_access_func(start_addr, machine, bus_read, &val, 4);
        if(print_addr)
            printf("0x%08x:", start_addr);
        printf("%08x\n", val);
        start_addr += 4;
    }
}