Design Goals
============

LDL should be simple to use and port.

LDL should provide minimal interfaces that line up with words used in the 
specification. More sophisticated interfaces should be implemented as wrappers.

LDL should not repeat itself when implementing regions. All LoRaWAN regions are based on one of two patterns; fixed or
programmable channels. LDL should take advantage of the overlap.

It should be possible to wrap LDL in another programming language.

It should be possible to run LDL with the hardware layer replaced by software.

LDL should be able to share a single thread of execution with other tasks. It should:

- not block
- indicate when it requires prioritisation
- indicate time until the next event
- calculate how late it is to handling an event
- compensate for timing jitter
- push events to the application asynchronously

LDL should fit into an existing system, the existing system should not 
have to fit into LDL.
