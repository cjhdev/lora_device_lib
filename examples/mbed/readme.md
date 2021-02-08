MBED Examples
=============

There are two example applications; one for bare-metal, one for RTOS.
These examples should compile for more targets and radios than I have
access to or an interest in testing.

The target can be changed as per any MBED project. The radio can be
changed by redefining RADIO macro, or leaving it undefined and uncommenting
the one you want in the source. It's setup so that I can easily check
different permutations of targets and radios.
If you don't want to use default radio hardware then you may need to
hack up the source a little.

The bare metal profile is essential for targets with less than 20K of RAM.

The rtos profile is preferable if you have enough RAM since the
blocking interfaces simplify applications and preemtive scheduling
reduces the chance you will miss downlinks.

There are two ways to compile the examples.

## Import as MBED App Using Preferred Tools

Treat each app as standalone and follow the usual instructions
for importing these into your preferred environment. Define RADIO or
uncomment the definition in source.

## Compile in Place using Docker

Docker is used to:

- hold mbed-cli and a toolchain
- mount different parts of the project repository where mbed-cli expects them

This is the method used to maintain the wrapper and examples.
The makefile in this directory can be used to setup the container and build
the examples.

### Checkout mbed-os

```
make get_mbed
```

### Build a Container

```
make build_container
```

You can check it works:

```
make test_container
```

### Compile

Compile for with predefined settings referenced by APP variable. This
should be the basename of a file in [targets](targets) directory.

e.g.

```
make compile APP=rtos_wl55
```
```
make compile APP=bm_wl55
```
```
make compile APP=bm_lrwan1
```

### Clean

```
make clean APP=bare_metal
```

### Flash

```
make flash APP=rtos_wl55
```
Define APP same as you would with compile.

### GDB

```
make gdb APP=rtos_wl55
```
Define APP same as you would with compile.

### GDB Server

```
make gdb_server APP=rtos_wl55
```
Define APP same as you would with compile.


