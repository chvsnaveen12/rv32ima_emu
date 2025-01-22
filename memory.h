#define ROM_BASE 0x1000
#define ROM_SIZE 0xf000

#define RAM_BASE 0x80000000
#define RAM_SIZE 1024*1024*128

#define UART_BASE 0x10000000
#define UART_SIZE 8

#define PLIC_BASE 0x0C000000
#define PLIC_SIZE 0x3FFF004

#define CLINT_BASE 0x02000000
#define CLINT_SIZE 0x000c0000

#define BLK_BASE 0x4200000
#define BLK_SIZE 0x200

#define SYSCON_BASE 0x11100000
#define SYSCON_SIZE 0x1000

#define MiB_16 0x01000000
#define FDT_ADDR (RAM_BASE + RAM_SIZE - MiB_16)
#define LINUX_ADDR 0x80400000
#define LINUX_MODE 0x00000001
#define INITRD_ADDR (RAM_BASE + RAM_SIZE - MiB_16*2)

// MMU
#define SV32_MODE 0x01
#define SV32_LEVELS 2
#define SATP_MODE_BIT 31
#define PPN_MASK 0x00000000003fffffUL
#define PAGE_SIZE_SHIFT 12
#define PTE_SIZE_SHIFT 2
#define VPN_MASK 0x3ff
#define PTE_RWX_MASK 0xe
#define OFFSET_MASK 0xfff

#define PTE_V 0
#define PTE_R 1
#define PTE_W 2
#define PTE_X 3
#define PTE_U 4
#define PTE_G 5
#define PTE_A 6
#define PTE_D 7