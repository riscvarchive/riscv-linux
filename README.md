Linux/RISC-V
================================================================================

This is a port of Linux kernel for the [RISC-V](http://riscv.org/)
instruction set architecture.
Development is currently based on the [3.14 longterm branch](
https://git.kernel.org/cgit/linux/kernel/git/stable/linux-stable.git/log/?h=linux-3.14.y).

Building the kernel image
--------------------------------------------------------------------------------

1. Fetch upstream sources and overlay the `riscv` architecture-specific
   subtree:

        $ curl -L https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.14.41.tar.xz | tar -xJ
        $ cd linux-3.14.41
        $ git init
        $ git remote add origin https://github.com/riscv/riscv-linux.git
        $ git fetch
        $ git checkout -f -t origin/master

1. Populate `./.config` with the default symbol values:

        $ make ARCH=riscv defconfig

1. Optionally edit the configuration via an ncurses interface:

        $ make ARCH=riscv menuconfig

1. Build the uncompressed kernel image:

        $ make ARCH=riscv -j vmlinux

1. Boot the kernel in the functional simulator, optionally specifying a
   raw disk image for the root filesystem:

        $ spike +disk=path/to/root.img bbl vmlinux

   `bbl` (the Berkeley Boot Loader) is available from the
   [riscv-pk](https://github.com/riscv/riscv-pk) repository.

Exporting kernel headers
--------------------------------------------------------------------------------

The `riscv-gnu-toolchain` repository includes a copy of the kernel header files.
If the userspace API has changed, export the updated headers to the
`riscv-gnu-toolchain` source directory:

    $ make ARCH=riscv headers_check
    $ make ARCH=riscv INSTALL_HDR_PATH=path/to/riscv-gnu-toolchain/linux-headers headers_install

Rebuild `riscv64-unknown-linux-gnu-gcc` with the `linux` target:

    $ cd path/to/riscv-gnu-toolchain
    $ make linux

