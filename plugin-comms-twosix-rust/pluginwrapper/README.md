# Plugin Wrapper

The plugin wrapper project is a Rust project responsible for wrapping a native Rust comms plugin in a C interface. This project is responsible for wrapping all API calls to be C compatible and converting all data types to C compatible data types.

Generally, a plugin author will not need to modify any of this code. It is advised that all the code in `pluginwrapper/src/race_wrappers` be left unmodified. However, two other modifications will need to be made.
1. You will need to modify `pluginwrapper/Cargo.toml` replace the example plugin dependency with that of your own implementation under the `[dependencies]` section.
2. You will need to modify `pluginwrapper/src/lib.rs` to use your plugin instead of the example. Look for this section and modify it.
    ```rust
    // Import the plugin to wrap.
    extern crate plugin_comms_twosix_rust;
    use plugin_comms_twosix_rust::plugin_comms_twosix_rust::PluginCommsTwoSixRust as CommsPlugin;
    ```

## Memory Management

Memory management needs to be considered very carefully. Any memory allocated by Rust must remain owned by Rust and eventually cleaned up by Rust. Memory allocated by C++ must be deallocated by C++. This can be tricky with more complex cases involving dynamic data types (strings, vectors, etc.). However, all of our use cases have been solved, although the code isn't the most beautiful thing in the world...

### Returning a Non C Type From Rust

While many of the C++ APIs return void (ah, nice and easy), some of them return complex data types that don't or can't exist in C. For example:

```cpp
virtual LinkProperties getLinkProperties(LinkType linkType, LinkID linkId) = 0;
```

The API above returns an instance of the `LinkProperties` class. C, of course, has no idea what a class is, and certainly can't return one. Furthermore, the memory returned by the function should be managed by the caller, meaning the function can't even allocate the memory to return, at least not without some help.

The current solution is to create C wrappers of these that return void and add an additional parameter: a pointer to the pre-allocated memory to be "returned".

```cpp
void plugin_get_link_properties(void *input, LinkPropertiesC *props, LinkType linkType,
                                const char *linkId);
```

The `LinkProperties` class is pre-allocated by the caller as a C compatible struct `LinkPropertiesC` (a C bridge between the C++ class and the Rust struct). This allows the caller to fully mangage the memory. For example:

```cpp
// Pre-allocate the memory for the struct.
LinkPropertiesC result;
// Pass in a pointer to the struct to be "returned".
plugin_get_link_properties(plugin, &result, linkType, linkId.c_str());

// Do some stuff with the struct.
LinkProperties props;
helper::convertLinkPropertiesCToClass(result, props);
```

#### Alternative Solutions

One alternative solution would be to provide Rust with helper functions to allocate memory in C++ for objects to be returned. However, by doing this, C++ now must be responsible for cleaning up the memory since the Rust function will have returned. Rust has no way of manually cleaning up the memory since the function that allocated the memory has completed. So, this approach isn't really appropriate for returning data. However, it does work well when passing data as arguments to API calls.
