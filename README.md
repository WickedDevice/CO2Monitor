CO2 sensor firmware
===================

*Not complete*

Firmware for a WildFire (V2) & K 30 CO2 sensor to interact with [this rails server](https://github.com/WickedDevice/CUCO2_Website)

CUCO2_batched is the current version of the WildFire sketch.

Organization of functions into the files is somewhat haphazard.

* `header.h` contains all of the parts of the configuration that is likely to change (at compile time).

* `CUCO2_batched.ino` - has loop, setup, and the configuration that is unlikely to change.

* `loop_functions.ino` - functions used in loop.

* `setup_functions.ino` - functions used in setup.

* `encryption.ino` - self contained encryption method using Vignere encryption.

* `memory_management.ino` contains all the functions related to memory. It should be entirely self-contained, & the implementation can be changed without modifying the rest of the program.
