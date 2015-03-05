# xlink
*data transfer and control system for the commodore 64*

xlink allows connecting a Commodore 64 to a PC through either a custom build USB adapter or a simple parallel port cable. A command-line client is used on the PC to transfer data to and from C64 memory, run programs on the C64, write or read from an attached disk drive or to initiate a hardware reset.

An interrupt-driven server on the Commode 64 listens to and executes the commands send by the client. The server can be temporarily loaded on the C64, or it can be permanently installed using a customized kernal rom. The latter has the advantage of being instantly available after power-up or reset, which makes the xlink system well suited for fast and easy cross development using a PC and a real Commodore 64.

The implementation of the underlying functionality is distributed as a shared library, making the functionality provided by xlink readily available for use in other programs.

The xlink client software and library is supported under Linux and Windows.

Please see the [project website](http://henning-bekel.de/xlink) for more information
