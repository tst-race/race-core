# Rust Bindgen Notes

## Workflow

Based on our codebase and what I've read across differeng guides and issues, the workflow for using Bindgen would look like this.

1. Create some C++ header to wrap (we already have this, racesdk-common, etc.).
2. (manual) Wrap said C++ header with a psuedo-C interface since it is heavily using STL. This can still be a class that only exposes C types, e.g. no STL. Also, virtual functions will not be wrapped.
3. (automatic) Run Bindgen on the new C interface to create Rust bindings.
4. (manual) Wrap the "ugly" Rust bindings is a more "Rust-y" style interface.
5. Build against the "Rust-y" interface.

I'd imagine a workflow without Bindgen to go something like:

1. Create some C++ header to wrap (we already have this, racesdk-common, etc.).
2. (manual) Wrap said C++ header with a C interface.
3. (manual) Wrap the C interface in a "Rust-y" style interface.
5. Build against the "Rust-y" interface.

Not really sure what value Bindgen is adding here... handles classes that contain no STL? structs? Neither of which really help our use case.

## Issues
* [MAJOR] Does not support pure virtual functions, does not know how to handle virtual function table. So `IRacePluginComms.h` and `IRaceSdkComms.h` would have to be handled manually.
  * [source](https://github.com/rust-lang/rust-bindgen/issues/1400)
* Issues with std::string
  > would need your own wrappers, at least for now. String inherits from basic_string<char_t> in all the libstd's I've known, and that's kinda hard to deal with, because we don't generate methods for template structs
  * [source](https://github.com/rust-lang/rust-bindgen/issues/598#issuecomment-289235271)
* Templates aren't supported, which rules out basically all STL (string, vector, map).
  * [source](https://rust-lang.github.io/rust-bindgen/cpp.html#unsupported-features)
  * [source](https://rust-lang.github.io/rust-bindgen/faq.html#does-bindgen-support-the-c-standard-template-library-stl)
* _Should_ wrap the generated Rust bindings in a "prettier" Rust interface.
  > This is a little wordy using the raw FFI bindings, but hopefully we wouldn't usually ask people to do this, we'd provide a nice Rust-y API on top of the raw FFI bindings for them.
  * [source](https://rust-lang.github.io/rust-bindgen/tutorial-5.html#write-a-sanity-test)
