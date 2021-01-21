MBED Examples
=============

- bare_metal: builds using bare metal profile
- rtos: builds using rtos profile

There are two ways to use these apps:

## Import as MBED App Using Preferred Tools

Treat each app as standalone and follow the usual instructions
for importing these into your preferred environment.

## Compile in Place using Docker

Docker is used to:

- hold mbed-cli and a toolchain
- mount different parts of the project repository where mbed-cli expects them

This is the method used to maintain the wrapper.
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

### Compile

Compile different apps by defining APP. The folder name is the APP name.

```
make compile APP=bare_metal
```
```
make compile APP=rtos
```
You can also change:

- DEVICE: this is the MBED device name
- JLINK_DEVICE: this is the name of the MCU for JLINK

### Clean

```
make clean APP=bare_metal
```

### Flash

```
make flash APP=bare_metal
```
Can use variables the same as with Compile.

### GDB

```
make gdb APP=bare_metal
```
Can use variables the same as with Compile.

### GDB Server

```
make gdb_server APP=bare_metal
```
Can use variables the same as with Compile.


