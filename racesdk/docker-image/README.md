# Race SDK Docker Image

## Building on M1 Mac

Currently there is no support for building x86 artifacts on M1 Macs. It is still possible to build RACE sdk Docker images, however they will only include ARM artifacts. To build the Docker image follows these steps (all paths are relative to the root of the project).

1. From inside a RACE compile container build the linux artifacts:
    ```bash
    ./build.sh --linux-arm64 --no-kits
    ./racesdk/create-package.sh
    ```
1. From your host machine build the Android artifacts:
    ```bash
    ./build.sh --android-arm64 --no-kits
    ./racesdk/create-package.sh
    ```
1. From you host machine build the image:
    ```bash
    ./build_sdk_image.sh --platform-arm64
    ```
