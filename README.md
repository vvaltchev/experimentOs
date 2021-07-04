Tilck (Tiny Linux-Compatible Kernel)
-------------------------------------

[![Build Status](https://vkvaltchev.visualstudio.com/Tilck/_apis/build/status/Tilck?branchName=master)](https://vkvaltchev.visualstudio.com/Tilck/_build/latest?definitionId=1&branchName=master)
[![codecov](https://codecov.io/gh/vvaltchev/tilck/branch/master/graph/badge.svg)](https://codecov.io/gh/vvaltchev/tilck)
[![License](https://img.shields.io/badge/License-BSD%202--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)

<a href="https://youtu.be/Ce1pMlZO_mI">
   <img
      align="right"
      src="http://vvaltchev.github.io/tilck_imgs/v2/main2-smaller-70p-yt.png"
      alt="Tilck"
   >
</a>

Contents
----------------------------------------
* [Overview](#overview)
  - [What is Tilck?](#what-is-tilck)
    * [Future plans](#future-plans)
  - [What Tilck is NOT ?](#what-tilck-is-not-)
* [Features](#features)
   - [Hardware support](#hardware-support)
   - [File systems](#file-systems)
   - [Processes and signals](#processes-and-signals)
   - [I/O](#io)
   - [Console](#console)
   - [Userspace applications](#userspace-applications)
   - [Screenshots](#screenshots)
* [Booting Tilck](#booting-tilck)
  - [Tilck's bootloader](#tilcks-bootloader)
  - [3rd-party bootloaders](#3rd-party-bootloaders)
    * [Grub support](#grub-support)
* [Documentation and HOWTOs](#documentation-and-howtos)
  - [Building Tilck](#building-tilck)
  - [Testing Tilck](#testing-tilck)
  - [Debugging Tilck](#debugging-tilck)
    * [Tilck's debug panel](#tilcks-debug-panel)
* [A comment about user experience](#a-comment-about-user-experience)
* [FAQ](#faq-by-vvaltchev)
  - [Why Tilck does not have the feature/abstraction XYZ?](#why-tilck-does-not-have-the-featureabstraction-xyz-like-other-kernels-do)
  - [Why Tilck runs only on x86 (ia-32)?](#why-tilck-runs-only-on-x86-ia-32)
  - [Why having support for FAT32?](#why-having-support-for-fat32)
  - [Why keeping the initrd mounted?](#why-keeping-the-initrd-mounted)
  - [Why using 3 spaces as indentation?](#why-using-3-spaces-as-indentation)
  - [Why many commit messages are so short?](#why-many-commit-messages-are-so-short)

Overview
----------------------------------------

<p align="center">
    <img src="http://vvaltchev.github.io/tilck_imgs/v2/overview.png" border="0">
</p>

### What is Tilck?
`Tilck` is an educational *monolithic* x86 kernel designed to be Linux-compatible at
binary level. Project's small-scale and simple design makes it the **perfect playground**
for playing in kernel mode while retaining the ability to compare how the *very same*
*usermode bits* run on the Linux kernel as well. That's a **unique feature** in the
realm of educational kernels. Thanks to that, to build a program for Tilck it's enough to
use a `i686-musl` toolchain from [bootlin.com](https://toolchains.bootlin.com). Tilck
has **no need** to have its own set of custom written applications, like most educational
kernels do. It just runs mainstream Linux programs like the **BusyBox** suite.
While the Linux-compatibility and the monolithic design might seem a limitation from
the OS research point of view, on the other side, such design bring the whole project
much closer to *real-world* applications in the future, compared to the case where
some serious (or huge) effort is required to port pre-existing software on it. Also,
nothing stops Tilck from implementing custom non-Linux syscalls that aware apps might
take advantage of.

#### Future plans
In the long term, depending on how successful the project will be, `Tilck` might
become suitable for **embedded systems** on which an extra-simple and fully deterministic
kernel is required or, at least, it is considered the optimal solution. With a fair
amount of luck, `Tilck` might be able to fill the gap between *Embedded Linux* and
typical real-time operating systems like *FreeRTOS* or *Zephyr*. In any case, at some
point it will be ported to the `ARM` family and it might be adapted to run on MMU-less
CPUs as well. Tilck would be a perfect fit for that because consuming a tiny amount of
RAM has always been a key point in Tilck's design. Indeed, the kernel can *comfortably*
boot and run on a i686 QEMU machine with just 4 MB of memory *today*. Of course, that's
pointless on x86, but on an ARM Cortex-R it won't be anymore the case.


### What Tilck is NOT ?
An attempt to re-write and/or replace the Linux kernel. Tilck is a completely
different kernel that has a *partial* compatibility with Linux just in order to
take advantage of its programs and toolchains. Also, that helps a lot to validate
its correctness: if a program works correctly on Linux, it must work the same way
on Tilck as well (except for the not-implemented features). **But**, having a fair
amount of Linux programs working on it, is just a starting point: after that, Tilck
will evolve in a different way and it will have its own unique set of features as
well. Tilck is fundamentally different from Linux in its design and its trade-offs
as it **does not** aim to target multi-user server or desktop machines. Currently,
it targets the educational world, while in the future it might target embedded
systems or something else.

Features
----------------------------------------

Tilck is a preemptable monolithic (but with compile-time modules) *NIX kernel,
implementing about ~100 Linux syscalls (both via `int 0x80` and `sysenter`) on
x86. At its core, the kernel is not x86-centric even if it runs only on x86 at
the moment. Everything arch-specific is isolated. Because of that, most of
kernel's code can be already compiled for any architecture and can be used in
kernel's unit tests.

#### Hardware support
While the kernel uses a fair amount of **legacy hardware** like the 8259 PICs for
IRQs, the legacy 8254 PIT for the system timer, the legacy 16550 UART for serial
communication, the 8042 kb controller, the 8237 ISA DMA, and the Sound Blaster
16 sound card (QEMU only), it has also support for some **recent hardware** features
like SSE, AVX and AVX2 fpu instructions, PAT, i686 sysenter, enumeration of PCI Express
devices (via ECAM) and, above all, **ACPI** support via ACPICA. ACPI is currently
used to receive power-button events, to reboot or power-off the machine, and to
read the current parameters of machine's batteries (when implemented via ACPI control
methods).

#### File systems
Tilck has a simple but full-featured (both soft and hard links, file holes, memory
mapping, etc.) **ramfs** implementation, a minimalistic **devfs** implementation,
read-only support for FAT16 and **FAT32** (used for initrd) allowing memory-mapping
of files, and a **sysfs** implementation used to provide a full view of **ACPI's**
**namespace**, the list of all PCI(e) devices and Tilck's compile-time configuration.
Clearly, in order to work with multiple file systems at once, Tilck has a simple
**VFS** implementation as well.

#### Processes and signals
While Tilck uses internally the concept of thread, multi-threading is not currently
exposed to userspace (kernel threads exist, of course). Both `fork()` and `vfork()` are
properly implemented and copy-on-write is used for fork-ed processes. The `waitpid()`
syscall is fully implemented (which implies process groups etc.). The support for
POSIX signals is partial: custom signal handlers are supported using the `rt_sigaction()`
interface, but most of the SA_* flags are not supported and handlers cannot interrupt
each other, yet. `rt_sigprocmask()`, `sys_rt_sigpending()`, `sys_rt_sigsuspend()`
work as expected, as well as special signals like SIGSTOP, SIGCONT and SIGCHLD.
For more details, see the [syscalls] document.

One interesting feature in this area deserves a special mention: despite the lack of
multi-threading in userspace, Tilck has full support for TLS (thread-local storage) via
`set_thread_area()`, because `libmusl` requires it, even for classic single-threaded
processes.

#### I/O
In addition to the classic `read()` and `write()` syscalls, Tilck supports vectored I/O
via `readv()` and `writev()` as well. In addition to that, non blocking I/O, `select()`
and `poll()` are supported too. Fortunately, no program so far needed `epoll` :-)

#### Console
Tilck has a console supporting more than 90% of Linux's console's features. It works
in the same way (using layers of abstraction) both in text mode and in framebuffer mode.
The effort to implement such a powerful console was driven by the goal to make **Vim** work
smoothly on Tilck, with syntax highlighting etc. While it's true that such a thing has a
little to do with "proper" kernel development, being able to run a "beast" like Vim on a
simple kernel like Tilck, is a great achievement by itself because it shows that Tilck
can run correctly programs having a fair amount of complexity.

#### Userspace applications
Tilck can run a fair amount of console applications like the **BusyBox** suite,
**Vim**, **TinyCC**, **Micropython**, **Lua**, and framebuffer applications like
a port of DOOM for the Linux console called **fbDOOM**. Check project's [wiki page]
for more info about that.

#### Screenshots
![Tilck screenshots](http://vvaltchev.github.io/tilck_imgs/v2/thumbnails.png)

For full-size screenshots and much more stuff, check Tilck's [wiki page].

[syscalls]: docs/syscalls.md
[wiki page]: https://github.com/vvaltchev/tilck/wiki

Booting Tilck
----------------------------------------

### Tilck's bootloader
`Tilck` comes with an interactive bootloader working both on legacy BIOS and on
UEFI systems as well. The bootloader allows the user to choose the desired video
mode, the kernel file itself and to edit kernel's cmdline.

![Tilck's bootloader](http://vvaltchev.github.io/tilck_imgs/v2/bootloader.png)

### 3rd-party bootloaders
`Tilck` can be loaded by any bootloader supporting `multiboot 1.0`. For example,
qemu's built-in bootloader works perfectly with `Tilck`:

    qemu-system-i386 -kernel ./build/tilck -initrd ./build/fatpart

Actually that way of booting the kernel is used in the system tests. A shortcut
for it is:

    ./build/run_multiboot_qemu

#### Grub support
`Tilck` can be easily booted with GRUB. Just edit your `/etc/grub.d/40_custom`
file (or create another one) by adding an entry like:

```
menuentry "Tilck" {
    multiboot <PATH-TO-TILCK-BUILD-DIR>/tilck
    module --nounzip <PATH-TO-TILCK-BUILD-DIR>/fatpart
    boot
}
```
After that, just run `update-grub` as root and reboot your machine.

Documentation and HOWTOs
--------------------------

Project's main documentation can be found in the `docs/` directory. However,
[Tilck's wiki](https://github.com/vvaltchev/tilck/wiki) can be used to
navigate through those documention files with the addition of much extra content
like screenshots. Here below, instead, there's a quick *starter* guide, focusing
on the most common scenarios.

### Building Tilck
The project supports a fair amount of build configurations and customizations
but building using its default configuration can be described in just a few
steps. The *only* true requirement for building Tilck is having a Linux
x86_64 host system or Microsoft's `WSL`. Steps:

* Enter project's root directory.
* Build the toolchain (just the first time) with: `./scripts/build_toolchain`
* Compile the kernel and prepare the bootable image with: `make -j`

At this point, there will be an image file named `tilck.img` in the `build`
directory. The easiest way for actually trying `Tilck` at that point is to run:
`./build/run_qemu`.

#### Running it on physical hardware
The `tilck.img` image is, of course, bootable on physical machines as well,
both on UEFI systems and on legacy ones. Just flush the image file with `dd`
to a usb stick and reboot your machine.

#### Other configurations
To learn much more about how to build and configure Tilck, check the [building]
guide in the `docs/` directory.

[building]: docs/building.md

### Testing Tilck
Tilck has **unit tests**, **kernel self-tests**, **system tests** (using the
syscall interface), and **automated interactive system tests** (simulating real
user input through QEMU's monitor) all in the same repository, completely
integrated with its build system. In addition to that, there's full code
coverage support and useful scripts for generating HTML reports (see the
[coverage] guide). Finally, Tilck is fully integrated with the [Azure Pipelines]
[CI], which validates each pushed branch with builds and test runs in a variety
of configurations. Kernel's coverage data is also uploaded to [CodeCov]. Below,
there are some basic instructions to run most of Tilck's tests. For the whole
story, please read the [testing] document.

[Azure Pipelines]: https://azure.microsoft.com/en-us/services/devops/pipelines
[CI]: https://en.wikipedia.org/wiki/Continuous_integration
[CodeCov]: https://codecov.io

#### Running Tilck's tests
Running Tilck's tests is extremely simple: it just requires to have `python 3`
installed on the machine. For the **self-tests** and the classic
**system tests**, run:

    <BUILD_DIR>/st/run_all_tests -c

To run the **unit tests** instead:

   * Install the [googletest] library (once) with:
     `./scripts/build_toolchain -s build_gtest`

   * Build the unit tests with: `make -j gtests`

   * Run them with: `<BUILD_DIR>/gtests`

To learn much more about Tilck's tests in general and to understand how to run
its *interactive* system tests as well, read the [testing] document.

[testing]: docs/testing.md
[googletest]: https://github.com/google/googletest

### Debugging Tilck
With `QEMU`'s integrated `GDB` server, it's possible to debug the Tilck kernel
with GDB almost as if it were a regular process. It just gets tricky when
context switches happen, but GDB cannot help with that. To debug it with GDB,
follow the steps:

  - (Optional) Prepare a debug build of Tilck, for a better debugging experience.

  - Run Tilck's VM with: `./build/run_nokvm_qemu` but, remain at the bootloader
    stage.

  - In a different terminal, run: `gdb ./build/tilck_unstripped`.

  - In GDB, run: `target remote :1234` to connect to QEMU's gdb server.

  - Set one or more breakpoints using commands like: `break kmain`.

  - Type `c` to allow execution to continue and boot the OS by pressing ENTER
    in the bootloader.

In order to make the debugging experience better, Tilck comes with a set of
**GDB scripts** (see `other/gdb_scripts`). With them, it's super-easy to list
all the tasks on the system, the handles currently opened by any given process
and more. In order to learn how to take advantage of those GDB scripts and anything
else related to debugging the Tilck project, check the [debugging] document.

[debugging]: docs/debugging.md

#### Tilck's debug panel

<img align="right"
src="http://vvaltchev.github.io/tilck_imgs/v2/screenshots/dp04.png"
alt="Tilck's debug panel" width="50%" height="50%">

Debugging Tilck with GDB while it's running inside a VM is very convenient, but
in other cases (e.g. Tilck on real hardware) we don't have GDB support. In
addition to that, even when the kernel is running inside a VM, there are some
features that are just much more convient to expose directly from the kernel
itself rather than through GDB scripts. One way to expose kernel info to
userspace is to use `sysfs`, but that's not necessarily the most convenient way
for everything (still, Tilck does have [sysfs] implementation), especially when
*interaction* with the kernel itself is needed for debugging purposes. To help
in those cases, a *debug panel* has been introduced inside Tilck itself. It
started as something like Linux's [Magic SysRq] which evolved in a sort of TUI
application with debug info plus tracing capabilities for user processes. In the
future, it will support some proper debugging features as well. To learn more
about it, check the the [debugging] document.

[sysfs]: https://github.com/vvaltchev/tilck/wiki/Tilck's-sysfs
[Magic SysRq]: https://en.wikipedia.org/wiki/Magic_SysRq_key

A comment about user experience
----------------------------------

Tilck particularly distinguishes itself from many open source projects in one
way: it really cares about the **user experience** (where "user" means
"developer"). It's not the typical super-cool low-level project that's insanely
complex to build and configure; it's not a project requiring 200 things to be
installed on the host machine. Building such projects may require hours or even
days of effort (think about special configurations e.g. building with a
cross-compiler). Tilck instead, has been designed to be trivial to build and
test even by inexperienced people with basic knowledge of Linux. It has a
sophisticated script for building its own toolchain that works on all the major
Linux distributions and a powerful CMake-based build system. The build of Tilck
produces an image ready to be tested with QEMU or written on a USB stick. (To
some degree, it's like what the `buildroot` project does for Linux, but it's
much simpler.) Finally, the project includes also scripts for running Tilck
on QEMU with various configurations (BIOS boot, UEFI boot, direct (multi-)boot
with QEMU's `-kernel` option, etc.).

#### Motivation
The reason for having the above mentioned features is to offer its users and
potential contributors a really **nice** experience, avoiding any kind of
frustration. Hopefully, even the most experienced engineers will enjoy a zero
effort experience. But it's not all about reducing the frustration. It's also
about _not scaring_ students and junior developers who might be just curious to
see what this project is all about and maybe eager to write a simple program for
it and/or add a couple of `printk()`'s here and there in their fork. Hopefully,
some of those people *just playing* with Tilck might actually want to contribute
to its development.

In conclusion, even if some parts of the project itself are be pretty complex,
at least building and running its tests **must be** something anyone can do.

[coverage]: docs/coverage.md

FAQ (by vvaltchev)
---------------------

#### Why Tilck does not have the feature/abstraction XYZ like other kernels do?

`Tilck` is **not** meant to be a full-featured production kernel. Please, refer to
Linux (or other kernels) for that. The idea for the moment was just to implement an
educational kernel able to run a class of Linux console applications on real hardware.
At some point in the future and **with a lot of luck**, `Tilck` might actually have a
chance to be used in production embedded environments (as mentioned above) but it will
still be *by design* limited in terms of features compared to Embedded Linux. `Tilck`
will always try to be different from Linux, simply because Linux is already great
per se and it does not make any sense trying to reimplement it. Instead, it's worth
trying to create something *new* while playing the "linux-compatibility card".
What I expect is `Tilck` to start "stealing" ideas from hard real-time kernels,
once it gets ported to ARM and MMU-less CPUs. But today, the project is not there yet.

#### Why Tilck runs only on x86 (ia-32)?

Actually Tilck runs only on x86 *for the moment*. The kernel was born as a purely
educational project and the x86 architecture was already very friendly to me at
the time. Moving from x86 usermode assembly to "kernel" mode (real-mode and the
transitions back and forth to protected mode for the bootloader) required quite an
effort, but it still was, in my opinion, easier than "jumping" directly into a
completely unknown (for me) architecture, like `ARM`. I've also considered writing
from the beginning a `x86_64` kernel running completely in `long mode` but I
decided to stick initially with the `i686` architecture for the following reasons:

* The `long mode` is, roughly, another "layer" added on the top of 32-bit
  protected mode: in order to have a full understanding of its complexity, I
  thought it was better to start first with its legacy.

* The `long mode` does not have a full support for segmentation, while I wanted
  to get confident with this technology as well.

* The `long mode` has a 4-level paging system, which is more complex to use that
  the classic 2-level paging supported by `ia-32` (it was better to start with
  something simpler).

* I never considered the idea of writing a kernel for desktop or server-class
  machines where supporting a huge amount of memory is a must. We already have
  Linux for that.

* It seemed to me at the time, that more online "starters" documentation existed
  (like the articles on https://wiki.osdev.org/) for `i686` compared to any other
  architecture.

Said that, likely I'll make `Tilck` to support also `x86_64` and being able to run
in `long mode` at some point but, given the long-term plans for it as a tiny kernel
for embedded systems, making it to run on `ARM` machines has priority over supporting
`x86_64`. Anyway, at this stage, making the kernel (mostly arch-independent code)
powerful enough has absolute priority over the support for any specific architecture.
`x86` was just a pragmatic choice for its first archicture.

#### Why having support for FAT32?

The first reason for supporting FAT32 (and FAT16) was that a FAT partition is required
for booting with UEFI. Therefore, it was convienent at the time to store there also
all the rest of the "initrd" files (init, busybox etc.). After the boot, `ramfs` is
mounted at root, while the FAT32 boot partition is mounted at /initrd. Part of the FAT32
code in the kernel is reused by the legacy bootloader in order to read the kernel
file from the boot partition. Today, the "initrd" files are NOT stored anymore in the
boot partition; there are two separate FAT partitions instead: `bootpart`, a
small partition containing just the kernel file and the EFI bootloaders, and
`fatpart`, a slightly bigger partition (depending on the configration) containing
the initial ramdisk files (e.g. busybox) instead. After the boot, `fatpart` remains
mounted at `/initrd`, while none of the contents of `bootpart` are kept.

#### Why keeping the initrd mounted?

To minimize the peak in memory usage during boot. Consider the idea of having
a `tgz` archive and having to extract all of its files in the root directory:
doing that will require, even for a short period of time, keeping both the archive
and all of its contents in memory. This is against Tilck's effort to reduce its
memory footprint as much as possible; part of project's goals is being able to
run on very limited systems.

#### Why using 3 spaces as indentation?

Long story. It all started after using that coding style for years at VMware.
Initially, it looked pretty weird to me, but at some point I felt in love with
the way code looked. I got convinced that 2 spaces are just not enough, while 4
spaces are somehow "too much". Therefore, when I started the project in 2016,
I decided to stick with the indentation size I liked most, even if I knew that
using 4 spaces would have been better for most people. Today, I'm not sure if
that was the right decision, but I still like the way Tilck's code looks.

#### Why many commit messages are so short?

It is well-known that all popular open source projects care about having good
commit messages and nice git history. It is an investment that at some point
pays off. A few years ago, I even wrote a [blog post] about that. The problem is
that such investment actually starts paying off only when multiple people
contribute to a project or the project is really *mature enough*. It took a
*long* time for me to start considering Tilck as *kind of* mature. Actually,
that's not even a binary value, it's a slow process instead: with time (and
commits!), the project matured even if it still has a long way to go. Therefore,
by looking at the commits from the initial one to today, it's possible to
observe how they improved, both from the *message* point of view and from the
*content* point of view as well. In particular, during the last ~1,000 commits I
started not only re-ordering commits but to split, edit, and squash them all the
time. Git's `add -p` became a friend too. That's because today Tilck is pretty
stable and it starts to be a medium-sized project with its ~94,000 *physical*
lines of code and a git history of ~4,900 commits. It deserves much more effort
on each commit, compared to the past.

At the beginning, Tilck was just a small experimental and unstable project on
which I worked alone in my free time. It had even a different name,
`ExperimentOS`. Its source was also subject to drastic changes very often and I
didn't have a clear roadmap for it either. It was an overkill to spend so much
effort on each commit message as if I were preparing it for the Linux kernel.
Tilck is still *obviously* not Linux so, don't expect to see 30+ lines of
description for EVERY commit message from now on, BUT, the quality is raising
through a gradual process and that's pretty natural. As other people start to
contribute to the project, we all will have to raise further the bar in order to
the collaboration to succeed and being able to understand each other's code
faster. Today, the project still doesn't have regular contributors other than
myself and that's why many commits still have short commit messages.

[blog post]: https://blogs.vmware.com/opensource/2017/12/28/open-source-proprietary-software-engineer
