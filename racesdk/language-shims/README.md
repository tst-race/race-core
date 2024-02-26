**This is the RACE Plugin Shim repository**

**Performer:** TwoSix Labs

## Build

### Build the shared libraries

The project uses CMake to generate build files. To generate build files and build the shared libraries run:

```
cd build
cmake ../source
cmake --build . # or just run make
```

### Directory structure
The project follows the CMake guidelines of having `source` and `build` directories and adds `include`, and `lib` directories.

#### `source` directory

All SWIG interface files (`.i` files), and language + Plugin specific loaders are located here

#### `build` directory

This is an empty directory that holds all the temporary CMake files and the output shared libraries (typically a .so). Do not add anything to this directory. The idea behind the build directory is that you can delete it and start over fresh.

#### `include` directory

All SWIG generated source files are placed into this directory.

#### `lib` directory

Stores all the external headers and libs from core.
WARNING: this directory may change pending team consensus.

## Using Python Bindings

To create a Python plugin, you will need the specific SWIG generated Python (networkManagerPluginBindings.py or commsPluginBindings.py), as well as the generated compiled library (either `_networkManagerPluginBindings.so` or `_commsPluginBindings.so`). For now, copy both the python and shared library to your Plugin's source directory before trying to run the client.

```
$ cp racesdk-language-shims/build/_networkManagerPluginBindings.so racesdk-language-shims/include/networkManagerPluginBindings.py plugin-network-manager-twosix-python/source/
$ ./client
```
