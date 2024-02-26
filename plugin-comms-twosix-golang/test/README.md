**This is a RACE Plugin Repository**

**Plugin type:** Comms
**Performer:** TwoSix Labs
**Variation:** Python

### Directory structure
The project follows the CMake guidelines of having `source` and `build` directories and adds `include`, `lib`, and `test` directories.

#### `source` directory

All project sournce code is stored here. Generally this will be .cpp, but internal headers may be stored here as well/

#### `build` directory

This is an empty directory that holds all the temporary CMake files and the output shared library (typically a .so). Do not add anything to this directory. The idea behind the build directory is that you can delete it and start over fresh.

#### `include` directory

All exported headers should be put in this directory. Internal headers should be placed in `source`.

#### `lib` directory

Stores all the external headers and libs from core.
WARNING: this directory may change pending team consensus.

#### `test` directory

Stores all source code for the test application.
