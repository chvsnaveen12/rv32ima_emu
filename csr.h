#define NUM_CSRS 4096
#define CSR_ADDR_MASK 0x00000fff

// User
#define FFLAGS 0x001
#define FRM 0x002
#define FCSR 0x003
#define CYCLE 0xc00
#define TIME 0xc01
#define INSTRET 0xc02
#define CYCLEH 0xc80
#define TIMEH 0xc81
#define INSTRETH 0xc82

#define HPMCOUNTERS 0xc03
#define HPMCOUNTERE 0xc1f
#define HPMCOUNTERHS 0xc83
#define HPMCOUNTERHE 0xc9f

// Supervisor trap setup
#define SSTATUS 0x100
#define SIE 0x104
#define STVEC 0x105
#define SCOUNTEREN 0x106

// Supervisor config
#define SENVCFG 0x10a

// Supervisor counter setup
#define SCOUNTINHIBIT 0x120

// Supervisor trap handling
#define SSCRATCH 0x140
#define SEPC 0x141
#define SCAUSE 0x142
#define STVAL 0x143
#define SIP 0x144

// Supervisor protection and translation
#define SATP 0x180



// Machine information registers
#define MVENDORID 0xf11
#define MARCHID 0xf12
#define MIMPID 0xf13
#define MHARTID 0xf14
#define MCONFIGPTR 0xf15

// Machine trap setup registers
#define MSTATUS 0x300
#define MSTATUSH 0x310
#define MISA 0x301
#define MEDELEG 0x302
#define MEDELEGH 0x312
#define MIDELEG 0x303
#define MIE 0x304
#define MTVEC 0x305
#define MCOUNTEREN 0x306

// Machine trap handling
#define MSCRATCH 0x340
#define MEPC 0x341
#define MCAUSE 0x342
#define MTVAL 0x343
#define MIP 0x344
#define MTINST 0x34a
#define MTVAL2 0x34b

// Machine config
#define MENVCFG 0x30a
#define MENVCFGH 0x31a
#define MSECCFG 0x747
#define MSECCFGH 0x757

// Machine memory protection
#define PMPCFGS 0x3a0
#define PMPCFGE 0x3af
#define PMPADDRS 0x3b0
#define PMPADDRE 0x3ef

// Machine counter and timers
#define MCYCLE 0xb00
#define MCYCLEH 0xb80
#define MINSTRET 0xb02
#define MINSTRETH 0xb82

// Pseudo MTIME
#define MTIME 0x7c0

#define MHPMCOUNTERS 0xb03
#define MHPMCOUNTERE 0xc1f
#define MHPMCOUNTERHS 0xb83
#define MHPMCOUNTERHE 0xc9f

// Machine counter setup
#define MCOUNTINHIBIT 0x320
#define MHPMEVENTS 0x323
#define MHPMEVENTE 0x33f
#define MHPMEVENTHS 0x723
#define MHPMEVENTHE 0x73f

// MASKS
// UBE doesn't exist
// VS doesn't exist
// TVM TW TSR ALL NOt supported
#define MSTATUS_MASK 0x000ff9aa
#define SSTATUS_MASK 0x000de122
#define MSTATUS_SIE_BIT 1
#define MSTATUS_MIE_BIT 3
#define MSTATUS_SPIE_BIT 5
#define MSTATUS_MPIE_BIT 7
#define MSTATUS_SPP_BIT 8
#define MSTATUS_MPP_BIT 11
#define MSTATUS_MPRV_BIT 17
#define MSTATUS_SUM_BIT 18
#define MSTATUS_MXR_BIT 19

#define MEDELEG_MASK 0x0000b3ff

#define EPC_MASK 0xfffffffe
#define TVEC_MASK 0xfffffffd

#define SIE_SIP_MASK 0x00000222
#define MIE_MIP_MASK 0x00000aaa
#define SIP_SSIP_MASK 0x00000002
#define SSIP_BIT 1
#define MSIP_BIT 3
#define STIP_BIT 5
#define MTIP_BIT 7
#define SEIP_BIT 9
#define MEIP_BIT 11

#define COUNTEREN_DEFAULT 0xffffffff
#define MISA_DEFAULT 0x40141101

// Mcoutneren and Scounteren revert to Ffffff because the access is
// Provided


