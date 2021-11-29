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

For Raspberry PI 4 in particular, there is another Dockerfile:

```
cd raspbian
```

And now run the `docker buildx build` command. It's gonna take a while!

# Local changes

If you have local changes to your code, you might want to create a docker image with those rather than cloning <https://github.com/crownstone/bluenet>.

In the Dockerfile change:

```
RUN git clone https://github.com/crownstone/bluenet
```

into (watch the trailing backslash!):

```
RUN git clone git://localhost/ bluenet
```

Navigate to your local bluenet repository and serve it locally:

```
git daemon --verbose --export-all --base-path=.git --reuseaddr --strict-paths .git/
```

We need a slightly more privileged builder for this:

```
docker buildx create --name image-builder-network-host-user --driver-opt network=host
docker buildx use image-builder-network-host-user
```

And then build (for example for the linux/arm/v7 platform):

```
docker buildx build --platform linux/arm/v7 -t crownstone .
```

Docker will at times cache a particular step, you can bust the cache for a particular step by doing something like this:

```
ARG CACHEBUST=1
RUN git clone -b some-branch git://localhost/ bluenet
```

And if you want to bust the cache just use a different number than 1 for `CACHEBUST`.

```
docker buildx build --platform linux/arm/v7 -t crownstone . --build-arg CACHEBUST=2
```

Change this again if you want to bust the cache again.

The Dockerfile in `raspbian/` has some other arguments that can be used. Check its contents, e.g.

```
docker buildx build --platform linux/arm/v7 -t crownstone . --build-arg BLUENET_BRANCH=some-dev-branch --build-arg GITHUB_REPOS='git://localhost/'
```

All this puts the result in the cache, with `--output` you can decide how to use the result. The option `--progress plain` just more plainly shows the building progress.

To remove all caching, etc. use `docker buildx prune`.

# State

If there's a simple way to host the built docker online, feel free to suggest so. Know that when you run from a docker,
you have to take care that your changes to the code are actually preserved and/or committed to the (or your) repository.
