# Loader

The loader is responsble for generating a shared object that can be loaded as a comms plugin by the RACE SDK.

The loader also wraps the RACE SDK C++ API in a C interface so that it interact with Rust over FFI.

This code should NOT be modified by plugin authors. Altering this code may render your plugin incompatible with the RACE SDK.
