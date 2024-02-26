# Building for Android arm64 on M1 Macs

## Install dependencies

Homebrew is required to install dependencies. Installation instructions can be found [here](https://brew.sh/#install).

```bash
brew install cmake
brew install clang-format
brew install swig
brew tap homebrew/cask-versions
brew install --cask zulu8
brew install maven
```

Install Rust from [here](https://www.rust-lang.org/tools/install).

Install Golang version 1.14.15 from [here](https://go.dev/dl/#go1.14.15) ([direct link](https://go.dev/dl/go1.14.15.darwin-amd64.pkg)).

Install Android Studio from [here](https://developer.android.com/studio/install).

Install NDK version `23.2.8568313` using the instructions [here](https://developer.android.com/studio/projects/install-ndk).

Create a symlink for the Android build tools:
```bash
sudo bash -c "mkdir -p /opt/android/build-tools && ln -s $HOME/Library/Android/sdk/build-tools/31.0.0 /opt/android/build-tools/default"
```

Run the setup script [`./setup.sh`](setup.sh).

## Build

From your localhost run:

```bash
./build.sh --android-arm64
```
