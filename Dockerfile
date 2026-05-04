FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    ca-certificates curl bash xz-utils python3 \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh \
    && mv bin/arduino-cli /usr/local/bin/arduino-cli

WORKDIR /work/RectColumn
COPY . .

RUN chmod +x build.sh && ./build.sh
