# Linux/RISC-V

This is a port of Linux kernel for the [RISC-V](http://riscv.org/)
instruction set architecture.
Development is currently based on the
[4.6 longterm branch](https://git.kernel.org/cgit/linux/kernel/git/stable/linux-stable.git/log/?h=linux-4.6.y).


## Checking Out
The kernel history is quite large, you may want to limit the branches and commit
history that you checkout:

        $ git clone --depth 50 --single-branch https://github.com/riscv/riscv-linux.git

## Obtaining the Toolchain
You will need the [RISC-V tools](https://github.com/riscv/riscv-tools) in order to build linux.
In addition to following the RISC-V tools README, you must build the linux version of the
gnu toolchain:

    $ cd path/to/riscv-gnu-toolchain
    $ make linux

## Building the kernel image

1. Create kernel configuration based on architecture defaults:

        $ make ARCH=riscv defconfig

1. Optionally edit the configuration via an ncurses interface:

        $ make ARCH=riscv menuconfig

1. Build the uncompressed kernel image:

        $ make -j4 ARCH=riscv vmlinux

1. Link the kernel to the Berkeley Boot Loader (bbl)

        $ cd /path/to/risv-tools/riscv-pk/build
        $ ../configure --with-payload=/path/to/riscv-linux/vmlinux --host=riscv64-unknown-linux-gnu
        $ make bbl
        
1. Boot the kernel in the functional simulator.

        $ spike bbl

## Booting from Initramfs
Block devices are not currently supported. If you want a useful linux, you will need to configure it to boot from an initramfs. 

1. Enable initramfs in the linux config:

        "General setup -> Initial RAM Filesystem..." (CONFIG_BLK_DEV_INITRD=y)

1. Use a text file to specify contents of the initramfs:

        "General setup -> Initramfs source files = initramfs.txt" (CONFIG_INITRAMFS_SRC=initramfs.txt)

The file riscv-linux/initramfs.txt should contain a list of files to include. Here is a basic one to start with:
        
        dir /dev 755 0 0
        nod /dev/console 644 0 0 c 5 1
        nod /dev/null 644 0 0 c 1 3
        dir /proc 755 0 0
        slink /init /bin/busybox 755 0 0
        dir /bin 755 0 0
        file /bin/busybox /path/to/busybox 755 0 0
        dir /sbin 755 0 0
        dir /usr 755 0 0
        dir /usr/bin 755 0 0
        dir /usr/sbin 755 0 0
        dir /etc 755 0 0
        file /etc/inittab inittab 644 0 0
        dir /lib 755 0 0

Be sure to change "/path/to/busybox..." to point to your busybox binary
(the RISC-V Tools README has instructions on
[building busybox](https://github.com/riscv/riscv-tools#building-busybox)).
You will also need a riscv-linux/inittab file:

        ::sysinit:/bin/busybox --install
        ::sysinit:/bin/mount -t devtmpfs devtmpfs /dev
        ::sysinit:/bin/mount -t proc proc /proc
        /dev/console::sysinit:-/bin/ash

Now when you build and run linux, it should boot from the internal initramfs.

## Exporting kernel headers

The `riscv-gnu-toolchain` repository includes a copy of the kernel header files.
If the userspace API has changed, export the updated headers to the
`riscv-gnu-toolchain` source directory:

    $ make ARCH=riscv headers_check
    $ make ARCH=riscv INSTALL_HDR_PATH=path/to/riscv-gnu-toolchain/linux-headers headers_install

Rebuild `riscv64-unknown-linux-gnu-gcc` with the `linux` target:

    $ cd path/to/riscv-gnu-toolchain
    $ make linux

