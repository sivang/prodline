Showing that Python can be used to handle very low level tasks, like firmware signing
byte opcode manipulation and binary processing to prepare new embedded units to leave
the factory.

I've written this to handle signing and writing firmware to new manufactured
power over etherner embedded MIPS devices.

I've used the urwid library for text UI. This does many thing automatically but
expects a barcode reader to be attached for some of its input.

Mainly here for backup and reference purposes, if you found it useful, let me know.
