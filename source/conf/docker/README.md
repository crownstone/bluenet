# Docker

Creation of a docker image is straightforward

```
docker build -t crownstone .
```

The docker runs as root. Hence in `Dockerfile` you see that `-DSUPERUSER_SWITCH=""` is used.

# Cross-platform

It is possible to also create dockers for other platforms.

```
sudo apt install -y qemu-user-static binfmt-support
docker buildx create --name image-builder-user
docker buildx use image-builder-user
docker buildx inspect --bootstrap
```

A Raspberry PI 4, for example, has an ARMv8 processor. You might be running Raspbian on the PI though. For the later PI models there's a precompiled kernel targeting the ARMv7 architecture. This runs fine on the PI 4 and allows them Raspbian people to distribute fewer images. Hence, if you create docker for this architecture, pick `linux/arm/v7`.

```
docker buildx build --platform linux/arm/v7 -t crownstone .
```

# State

If there's a simple way to host the built docker online, feel free to suggest so. Know that when you run from a docker,
you have to take care that your changes to the code are actually preserved and/or committed to the (or your) repository.
