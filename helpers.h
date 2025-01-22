#include<stdint.h>
#include<stdbool.h>
#include<stdlib.h>

#define extract_bits(val, a, b) (((val) >> (b)) & ((1UL << ((a)-(b)+1)) - 1))
#define sext_bits(a) (0xffffffff << (a))

typedef enum priv_level{
    user = 0,
    supervisor = 1,
    hypervisor = 2,
    machine = 3,
    priv_max = 4
} priv_level;

typedef enum bus_access{
    bus_read = 0,
    bus_write = 1,
    bus_fetch = 2,
    MAX = 3
} bus_access;

typedef enum trap_cause_interrupt{
    rsvd_0 = 0,
    supervisor_swi,
    rsvd_1,
    machine_swi,
    rsvd_2,
    supervisor_ti,
    rsvd_3,
    machine_ti,
    rsvd_4,
    supervisor_exti,
    rsvd_5,
    machine_exti
} trap_cause_interrupt;

typedef enum trap_cause_exception{
    instr_addr_misalign = 0,
    instr_access_fault,
    illegal_instr,
    breakpoint,
    load_addr_misalig,
    load_access_fault,
    str_amo_addr_misalign,
    str_amo_access_fault,
    user_ecall,
    supervisor_ecall,
    rsvd_6,
    machine_ecall,
    instr_page_fault,
    load_page_fault,
    rsvd_7,
    str_amo_page_fault
} trap_cause_exception;

// Core
void core_init();
void core_dump();
void core_memory_dump(uint32_t start_addr, uint32_t end_addr, bool print_addr);
bool core_fetch(uint32_t* inst);
void core_decode(uint32_t inst);
void core_execute();
void core_process_interrupts(bool mei, bool sei, bool mti, bool msi);
void core_prepare_sync_trap(uint32_t cause, uint32_t tval);

// CSR
bool csr_read(priv_level priv, uint16_t addr, uint32_t *val);
bool csr_write(priv_level priv, uint16_t addr, uint32_t val);
void csr_set_mip(uint8_t pos);
void csr_clear_mip(uint8_t pos);

// Memory
void memory_init(char* sbi_file, char* linux_file, char* initrd_file, char* dtb_file);
bool memory_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len);
bool ram_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len);
bool soc_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len);
uint8_t* memory_get_ram_ptr();

// SOC
void soc_init(char* kernel_file, char* linux_file, char* initrd_file, char* dtb_file);
void soc_run();

// UART
bool uart_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len);
void uart_init();
bool uart_update();

// Helpers
void helper_write_from_file(char *fname, uint8_t *mem_ptr, uint64_t size);
uint64_t helper_get_file_size(char *fname);

// PLIC
bool plic_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len);
void plic_update_pending(uint32_t id, bool val);
void plic_update(bool* mei, bool* sei);

// CLINT
bool clint_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len);
void clint_update(bool* msi, bool* mti);
void clint_set_mtime(uint64_t val);
uint64_t clint_get_mtime();
