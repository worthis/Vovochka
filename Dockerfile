FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

# 1. Базовые инструменты и MinGW (для Windows)
RUN apt-get update && apt-get install -y \
    build-essential git cmake wget gpg apt-transport-https \
    mingw-w64 && rm -rf /var/lib/apt/lists/*

# 2. Установка devkitPro (для Switch)
RUN mkdir -p /usr/share/keyrings/ && \
    ln -sf /proc/mounts /etc/mtab && \
    wget -U "dkp-apt" -O /usr/share/keyrings/devkitpro-pub.gpg https://apt.devkitpro.org/devkitpro-pub.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/devkitpro-pub.gpg] https://apt.devkitpro.org stable main" > /etc/apt/sources.list.d/devkitpro.list && \
    apt-get update && \
    apt-get install -y devkitpro-pacman && \
    dkp-pacman -Syu --noconfirm && \
    dkp-pacman -S --noconfirm switch-dev libnx switch-portlibs switch-tools && \
    rm -rf /var/lib/apt/lists/*

ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=${DEVKITPRO}/devkitARM
ENV PATH=${DEVKITPRO}/tools/bin:$PATH

# 3. Raylib для Windows — master (6.1-dev)
RUN git clone https://github.com/raysan5/raylib.git /tmp/raylib-win && \
    cd /tmp/raylib-win && \
    git checkout caadb48e259028233515777a3e6402040c497309 && \
    cd src && \
    make PLATFORM=PLATFORM_DESKTOP CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar OS=Windows_NT && \
    mkdir -p /opt/raylib/win/lib /opt/raylib/win/include && \
    cp libraylib.a /opt/raylib/win/lib/ && \
    cp *.h /opt/raylib/win/include/ && \
    rm -rf /tmp/raylib-win

# 4. Raylib для Switch (luizpestana/raylib-nx, 6.1-dev)
RUN git clone https://github.com/luizpestana/raylib-nx.git /tmp/raylib-nx && \
    cd /tmp/raylib-nx && \
    git checkout 3afaa5a1fb5f690e66e0783f45455a8d93be54b6 && \
    cd src && \
    make PLATFORM=PLATFORM_NX && \
    mkdir -p /opt/raylib/switch/lib /opt/raylib/switch/include && \
    cp libraylib.a /opt/raylib/switch/lib/ && \
    cp *.h /opt/raylib/switch/include/ && \
    rm -rf /tmp/raylib-nx

WORKDIR /workspace