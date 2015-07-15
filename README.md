# xlink
*data transfer and control system for the commodore 64/128*

xlink allows connecting a Commodore 64 or 128 to a PC through either a
custom build USB adapter or a simple parallel port cable. A
command-line client is used on the PC to transfer data to and from the
remote machine memory, run programs on the remote machine or to
initiate a hardware reset.

An interrupt-driven server on the remote machine listens to and
executes the commands send by the client. The server can be
temporarily loaded on the remote machine, or it can be permanently
installed using a customized kernal rom. The latter has the advantage
of being instantly available after power-up or reset, which makes the
xlink system well suited for fast and easy cross development using a
PC and a real Commodore machine.

The implementation of the underlying functionality is distributed as a
shared library, making the functionality provided by xlink readily
available for use in other programs.

The xlink client software and library is supported under Linux and
Windows.

Please see the [project website](http://henning-bekel.de/xlink) for
more information
