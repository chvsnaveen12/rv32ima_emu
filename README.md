# rv32ima_emu
A RISC-V RV32IMA_Zicsr_Zifenci emulator capable of booting linux. Disk is supported via virtio-blk.
This has a fully functional PLIC, CLINT, ns16550 UART, SYSCON POWEROFF and VIRTIO-BLK. The boot image is a busybox rootfs with a simple init which just starts the shell.
The SBI is currently openSBI.

## Usage
```
git clone https://github.com/chvsnaveen12/rv32ima_emu
cd rv32ima_emu
make
./emu openSBI_IMAGE LINUX_IMAGE DISK_IMAGE FDT_FILE
```
This should drop you into a busybox rootfs without an init.

## Exiting
Since I haven't implemented a proper init system. The init file is essentially
```
!#/bin/sh
/bin/sh
```
A safe poweroff can be accomplished via this command
```
sync && poweroff -f
```
Although there should be no background processes running for this to work.
A forceful exit can be accomplished via pressing `Ctrl+A`

## Acknowledgements
A lot of inspiration has been drawn from https://github.com/franzflasch/riscv_em and https://github.com/sysprog21/semu
