# Package

## Location

This docker image is published at <https://github.com/orgs/crownstone/packages/container/package/bluenet>.

## Contents

The image has a `/bluenet` directory which is at a particular point in time.
It is possible to use this by something like:

```
run: cd /bluenet && git pull
```

However, for github workflows just use actions/checkout@v2 and check out what you are working on.

The reason we have checked out the repository is because it contains convenient helper scripts that download dependencies for us.

To get access to the downloaded tools, make a symlink.

```
run: ln -s /bluenet/tools tools
```

This makes for example the arm-none-eabi-gcc compiler available. If matters change with respect to which version of a compiler has to be used, this image has to be rebuilt.

## Generation

It is created by something like:

```
echo $CR_PAT | docker login ghcr.io -u $GITHUB_USERNAME --password-stdin
docker-squash --tag bluenet ghrc.io/crownstone/bluenet:latest
docker tag bluenet:latest ghcr.io/crownstone/bluenet:latest
docker push ghcr.io/crownstone/bluenet:latest
```

I had to squash the image, otherwise I ran into interaction limits.
