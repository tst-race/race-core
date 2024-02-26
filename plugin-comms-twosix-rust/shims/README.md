# Shims

The shims project contains Rust equivalents of the C++ API code which can be found [here](../../racesdk/common/include/).
This code was written by hand (i.e. not auto-generated from the C++ source). While it my not line up exactly with the equivalent C++ classes, all the available APIs should be available here as are in C++. If you notice any inconsistencies or limitations with these APIs compared to those of C++ please report the issue to Two Six Labs.

## Notes

### Project Content

This project should only contain RACE API code. This code should be generic for use by any and all performers. If you are writing your own plugin, do not modify this code. This code should not contain any plugin specific code.
