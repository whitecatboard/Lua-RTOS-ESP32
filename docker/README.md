# Image for build firmware using Docker

If you don't know how Docker works - please don't use it before read docs.docker.com for understanding how to use container and volumes mapping.

## How to build image

Clone repository and run
```
cd docker/
docker build -t any_name:1.0.0 .
```

Wait until process will be finished.

## How to build firmware

In this example used **/tmp** folder but you can use any other folders.

Run container with volume mapping

```
docker run -it -v /tmp/lua-docker-folder:/tmp/lua-docker-folder any_name:1.0.0 bash
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

Also, you can update code using `git pull` in /tmp/lua-docker-folder/source/

## How to firmware from container directly

Run container with volume and device mapping

```docker run -it -v /tmp/lua-docker-folder:/tmp/lua-docker-folder --device /dev/ttyUSB0:/dev/ttyUSB0 registry.gitlab.com/sanekz13/artifacts/whitecat-rtos-builder:1.0.0 bash```

After run execute command `chown -R builder:builder /dev/ttyUSB0` for provide access to not-root user (builder).

In this case you can use `make flash` for directly firmware your device
