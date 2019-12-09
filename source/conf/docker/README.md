# Docker

Creation of a docker image is straightforward

```
docker build -t crownstone .
```

The docker runs as root. Hence in `Dockerfile` you see that `-DSUPERUSER_SWITCH=""` is used.

# State

If there's a simple way to host the built docker online, feel free to suggest so. Know that when you run from a docker,
you have to take care that your changes to the code are actually preserved and/or committed to the (or your) repository.
