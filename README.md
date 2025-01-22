# rv32ima_emu
A RISC-V RV32IMA_Zicsr_Zifenci emulator capable of booting linux. Disk is supported via virtio-blk.
This has a fully functional PLIC, CLINT, ns16550 UART, SYSCON POWEROFF and VIRTIO-BLK. The boot image is a busybox rootfs with a simple init which just starts the shell.
The SBI is currently openSBI.

## Usage
```
git clone https://github.com/chvsnaveen12/rv32ima_emu
cd rv32ima_emu
make
./emu fw_dynamic_rv32ima.bin Image boot.img riscv_em32_linux.dtb
```
This should drop you into a busybox rootfs without an init.

## Poweroff
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

