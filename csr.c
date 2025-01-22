#include<stdio.h>
#include"csr.h"
#include"memory.h"
#include"helpers.h"

// User only has read perms, suepr and machine have rw perms

uint32_t csrs[NUM_CSRS] = { 0 };

bool csr_read(priv_level priv, uint16_t addr, uint32_t *val){
    uint64_t temp;

    if(priv == machine){
        if(addr == MVENDORID    || addr == MARCHID ||
           addr == MIMPID       || addr == MHARTID ||
           addr == MCONFIGPTR){
            *val = 0;
            return true;
        }

        // else if(PMPCFGS <= addr && addr <= PMPCFGE){
        //     *val = 0;
        //     return true;
        // }

        // else if(PMPADDRS <= addr && addr <= PMPADDRE){
        //     *val = 0;
        //     return true;
        // }
        
        // else if(MHPMCOUNTERS <= addr && addr <= MHPMCOUNTERE){
        //     *val = 0;
        //     return true;
        // }
        // else if(MHPMEVENTS <= addr && addr <= MHPMEVENTE){
        //     *val = 0;
        //     return true;
        // }
        // else if(MHPMCOUNTERHS <= addr && addr <= MHPMCOUNTERHE){
        //     *val = 0;
        //     return true;
        // }
        // else if(MHPMEVENTHS <= addr && addr <= MHPMEVENTHE){
        //     *val = 0;
        //     return true;
        // }

        switch(addr){
            case MSTATUS:
                *val = (csrs[MSTATUS] & MSTATUS_MASK);
                return true;
            // case MSTATUSH:
            //     *val = 0;
            //     return true;
            case MISA:
                *val = MISA_DEFAULT;
                return true;
            case MEDELEG:
                *val = csrs[MEDELEG];
                return true;
            // case MEDELEGH:
            //     *val = 0;
            //     return true;
            case MIDELEG:
                *val = csrs[MIDELEG];
                return true;
            case MIE:
                *val = csrs[MIE];
                return true;
            case MTVEC:
                *val = csrs[MTVEC];
                return true;
            // case MCOUNTEREN:
            //     *val = COUNTEREN_DEFAULT;
            //     return true;

            case MSCRATCH:
                *val = csrs[MSCRATCH];
                return true;
            case MEPC:
                *val = csrs[MEPC];
                return true;
            case MCAUSE:
                *val = csrs[MCAUSE];
                return true;
            case MTVAL:
                *val = csrs[MTVAL];
                return true;
            case MIP:
                *val = csrs[MIP];
                return true;
            // case MTINST:
            //     *val = csrs[MTINST];
            //     return true;
            // case MTVAL2:
            //     *val = csrs[MTVAL2];
            //     return true;

            // case MENVCFG:
            //     *val = 0;
            //     return true;
            // case MENVCFGH:
            //     *val = 0;
            //     return true;
            // case MSECCFG:
            //     *val = 0;
            //     return true;
            // case MSECCFGH:
            //     *val = 0;
            //     return true;

            // case MCYCLE:
            //     *val = csrs[MCYCLE];
            //     return true;
            // case MINSTRET:
            //     *val = csrs[MINSTRET];
            //     return true;

            // case MCOUNTINHIBIT:
            //     *val = 0;
            //     return true;
        }
    }
    
    if(priv == machine || priv == supervisor){
    // if(priv == supervisor){
        switch(addr){
            case SSTATUS:
                *val = (csrs[MSTATUS] & SSTATUS_MASK);
                return true;
            case SIE:
                *val = csrs[MIE] & csrs[MIDELEG];
                return true;
            case STVEC:
                *val = csrs[STVEC];
                return true;
            case SCOUNTEREN:
                *val = COUNTEREN_DEFAULT;
                return true;
            // case SENVCFG:
            //     *val = 0;
            //     return true;
            case SSCRATCH:
                *val = csrs[SSCRATCH];
                return true;
            case SEPC:
                *val = csrs[SEPC];
                return true;
            case SCAUSE:
                *val = csrs[SCAUSE];
                return true;
            case STVAL:
                *val = csrs[STVAL];
                return true;
            case SIP:
                *val = csrs[MIP] & csrs[MIDELEG];
                return true;
            case SATP:
                *val = csrs[SATP];
                return true;
        }
    }
    
    // if(priv == machine || priv == supervisor || priv == user){
    //     if(HPMCOUNTERS  <= addr && addr <= HPMCOUNTERE){
    //         *val = 0;
    //         return true;
    //     }
    //     else if(HPMCOUNTERHS  <= addr && addr <= HPMCOUNTERHE){
    //         *val = 0;
    //         return true;
    //     }

    //     switch(addr){
    //         case CYCLE:
    //             *val = csrs[MCYCLE];
    //             return true;
    //         case TIME:
    //             temp = clint_get_mtime();
    //             *val = (uint32_t)temp;
    //             return true;
    //         case INSTRET:
    //             *val = csrs[MINSTRET];
    //             return true;
    //         case CYCLEH:
    //             *val = csrs[MCYCLEH];
    //             return true;
    //         case TIMEH:
    //             temp = clint_get_mtime();
    //             *val = (uint32_t)(temp >> 32);
    //             return true;
    //         case INSTRETH:
    //             *val = csrs[MINSTRETH];
    //             return true;
        // }
    // }
    return false;
}

bool csr_write(priv_level priv, uint16_t addr, uint32_t val){
    uint64_t temp = 0;
    if(priv == machine){
        // if(PMPCFGS <= addr && addr <= PMPCFGE)
        //     return true;
        // if(PMPADDRS <= addr && addr <= PMPADDRE)
        //     return true;
        
        // if(MHPMCOUNTERS <= addr && addr <= MHPMCOUNTERE)
        //     return true;
        // if(MHPMEVENTS <= addr && addr <= MHPMEVENTE)
        //     return true;

        switch(addr){
            case MSTATUS:
                uint64_t temp = csrs[MSTATUS];
                temp &= (val | ~MSTATUS_MASK);
                temp |= (val &  MSTATUS_MASK);
                csrs[MSTATUS] = temp;
                return true;

            case MISA:
                return true;
            case MEDELEG:
                csrs[MEDELEG] = val & MEDELEG_MASK;
                return true;
            case MIDELEG:
                csrs[MIDELEG] = val & SIE_SIP_MASK;
                return true;
            case MIE:
                temp = csrs[MIE];
                temp &= (val | ~MIE_MIP_MASK);
                temp |= (val &  MIE_MIP_MASK);
                csrs[MIE] = temp;
                return true;
            case MTVEC:
                csrs[MTVEC] = val & TVEC_MASK;
                return true;
            // case MCOUNTEREN:
            //     return true;

            case MSCRATCH:
                csrs[MSCRATCH] = val;
                return true;
            case MEPC:
                csrs[MEPC] = val & EPC_MASK;
                return true;
            case MCAUSE:
                csrs[MCAUSE] = val;
                return true;
            case MTVAL:
                csrs[MTVAL] = val;
                return true;
            case MIP:
                temp = csrs[MIP];
                temp &= (val | ~SIE_SIP_MASK);
                temp |= (val &  SIE_SIP_MASK);
                csrs[MIP] = temp;
                return true;
            // case MTINST:
            //     csrs[MTINST] = val;
                // return true;
            // case MTVAL2:
            //     csrs[MTVAL2] = val;
            //     return true;

            // case MENVCFG:
            //     return true;
            // case MENVCFGH:
            //     return true;
            // case MSECCFG:
            //     return true;
            // case MSECCFGH:
            //     return true;

            // case MCYCLE:
            //     csrs[MCYCLE] = val;
            //     return true;
            // case MINSTRET:
            //     csrs[MINSTRET] = val;
            //     return true;
            
            // case MCOUNTINHIBIT:
            //     return true;
        }

    }

    if(priv == machine || priv == supervisor){
    // if(priv == supervisor){
        switch(addr){
            case SSTATUS:
                temp = csrs[MSTATUS];
                temp &= (val | ~SSTATUS_MASK);
                temp |= (val &  SSTATUS_MASK);
                csrs[MSTATUS] = temp;
                return true;
            case SIE:
                temp = csrs[MIE];
                temp &= (val | ~SIE_SIP_MASK);
                temp |= (val &  SIE_SIP_MASK);
                csrs[MIE] = temp;
                return true;
            case STVEC:
                csrs[STVEC] = val & TVEC_MASK;
                return true;
            case SCOUNTEREN:
                return true;
            // case SENVCFG:
            //     return true;
            case SSCRATCH:
                csrs[SSCRATCH] = val;
                return true;
            case SEPC:
                csrs[SEPC] = val & EPC_MASK;
                return true;
            case SCAUSE:
                csrs[SCAUSE] = val;
                return true;
            case STVAL:
                csrs[STVAL] = val;
                return true;
            case SIP:
                uint64_t temp = csrs[MIP];
                temp &= (val | ~SIP_SSIP_MASK);
                temp |= (val &  SIP_SSIP_MASK);
                csrs[MIP] = temp;
                return true;
            case SATP:
                if(val != 0 && (val >> SATP_MODE_BIT) != SV32_MODE)
                    return true;
                csrs[SATP] = val;
                return true;
        }
    }
    return false;
}

void csr_set_mip(uint8_t pos){
    csrs[MIP] |=  ((1 << pos) & MIE_MIP_MASK);
}

void csr_clear_mip(uint8_t pos){
    csrs[MIP] &= ~((1 << pos) & MIE_MIP_MASK);
}