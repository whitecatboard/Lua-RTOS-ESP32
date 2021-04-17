FROM ubuntu:18.04

RUN useradd -ms /bin/bash builder

RUN apt update
RUN apt-get install curl git wget make libncurses-dev flex bison gperf python python-pip python-serial python-future python-cryptography python-pyparsing picocom mc htop nano vim -y

RUN curl http://downloads.whitecatboard.org/console/linux/wcc --output /usr/local/bin/wcc && chmod +x /usr/local/bin/wcc

USER builder

RUN cd ~ && wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
RUN mkdir ~/esp && cd ~/esp && tar xzf ~/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
RUN cd ~ && git clone --recursive https://github.com/espressif/esp-idf.git
RUN /usr/bin/python -m pip install --user -r ~/esp-idf/requirements.txt
RUN mkdir -p /home/builder/ && cd /home/builder/ && git clone --recursive https://github.com/whitecatboard/Lua-RTOS-ESP32
ADD build.sh /home/builder/build.sh
RUN bash /home/builder/build.sh
ADD .bashrc /home/builder/.bashrc

USER root
