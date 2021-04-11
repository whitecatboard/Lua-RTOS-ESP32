# Image for build firmware using Docker

If you don't know how Docker works - please don't use it before read docs.docker.com for understanding how to use container and volumes mapping.

## How to build image

Clone repository and run
```
docker build -t any_name:1.0.0 .
```

## How to build firmware
Run container with volume mapping

```
docker run -it /tmp/lua-docker-folder:/tmp/lua-docker-folder any_name:1.0.0 bash
```

In container console run next commands
```
mkdir -p /tmp/lua-docker-folder/source/
su builder
cp -r /home/builder/Lua-RTOS-ESP32/ /tmp/lua-docker-folder/source/
cd /tmp/test/source/
```

After that you can configure your parameters using `make menuconfig` and `make`.

All changes will be in folder /tmp/lua-docker-folder/source/ on your local system.
