# kernel

A hobby operating system for x86-64, written in C++20 and NASM assembly. Developed and tested under QEMU.

## Overview

A monolithic kernel that exports Unix-like services to userland through a syscall interface. It provides preemptive multitasking with kernel and user threads, SMP support, per-process virtual memory with copy-on-write `fork` and demand-paged file mappings, and a virtual filesystem backed by an ext2 root filesystem.

On top of it sits a small userland: a C library, coreutils, a shell, and a few bundled ports. Drivers cover the hardware QEMU emulates, which is also the only machine it is tested on.

## Building

The kernel is cross-compiled with a patched GCC and binutils targeting `x86_64-corn`, so the toolchain has to be built first. `toolchain/` holds the build scripts and the patches that add the `corn` target; the binutils and GCC sources are downloaded by the scripts.

You need `curl`, `rsync`, `patch`, `make`, NASM, CMake ≥ 3.22 and Ninja.

```sh
cd toolchain
./build.sh                # builds x86_64-corn-gcc/g++ into ~/opt/cross,
                          # and copies the libc/kernel headers into root/usr/include
export PATH="$HOME/opt/cross/bin:$PATH"

cd ..
cmake -B build -G Ninja
cmake --build build
```

The toolchain build compiles all of GCC, so it may take a while the first time. The kernel image is written to `build/kernel/kernel.bin`.

## Booting

There are three boot paths. The first is the default and the only one that is regularly tested:

- **Multiboot loader** (`kernel/loader`) — `scripts/run.py` passes it to QEMU as `-kernel loader.bin -initrd kernel.bin`. It starts in 32-bit protected mode, builds the initial page tables, switches to long mode, and jumps into the kernel ELF.
- **UEFI** (`kernel/efi`) — boots through OVMF with `--uefi`. It works (hopefully), but is rarely exercised.
- **Limine** — `scripts/mkfs.limine.sh` installs Limine on the image and boots `kernel.bin` directly over the limine protocol. Also rarely exercised.

## Running

Build a disk image first; this loop-mounts and formats it, so it needs root. The loader expects the ext2 root filesystem on the image's second partition:

```sh
sudo python3 scripts/mkfs.limine.sh    # or: cmake --build build --target image
```

Then boot it:

```sh
python3 scripts/run.py                 # or: cmake --build build --target run
```

Run `python3 scripts/run.py --help` for the full list of options.

## License

MIT — see [LICENSE](LICENSE).
