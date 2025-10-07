#
#    Dockerfile for the creation of the logue SDK development environment docker image
#

#
#    BSD 3-Clause License
#
#    Copyright (c) 2022, KORG INC.
#    All rights reserved.
#
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice, this
#      list of conditions and the following disclaimer.
#
#    * Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#
#    * Neither the name of the copyright holder nor the names of its
#      contributors may be used to endorse or promote products derived from
#      this software without specific prior written permission.
#
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

FROM ubuntu:20.04 AS build
ARG build
ARG version

LABEL stage=builder \
      jp.co.korg.vendor="KORG Inc." \
      jp.co.korg.imgid=logue-sdk-dev-env \
      build=$build \
      version=$version
      
ENV ENV_BUILD=$build \
    ENV_VERSION=$version \
    DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

# Install required packages
RUN apt-get update && apt-get upgrade -y && apt-get install -y coreutils sudo make diffutils patch gawk curl uuid-runtime less vim nano xxd file tar zip unzip jq gcc-arm-none-eabi && apt-get clean

# Common files
COPY ./profile /root/.profile
COPY ./env /app/env
COPY ./interactive_entry /app/
COPY ./cmd_entry /app/
COPY ./VERSION /app/
COPY ./commands/* /app/commands/

# drumlogue specific files
COPY ./drumlogue/environment /app/drumlogue/
COPY ./builder/get_drumlogue_toolchain.sh /app/drumlogue/
RUN /bin/bash -c "/app/drumlogue/get_drumlogue_toolchain.sh && rm /app/drumlogue/get_drumlogue_toolchain.sh"

# minilogue-xd specific files
COPY ./minilogue-xd/environment /app/minilogue-xd/

# nts-1 specific files
COPY ./nts-1/environment /app/nts-1/

# nts-1 mkii specific files
COPY ./nts-1_mkii/environment /app/nts-1_mkii/

# nts-3 specific files
COPY ./nts-3_kaoss/environment /app/nts-3_kaoss/

# prologue specific files
COPY ./prologue/environment /app/prologue/

# microkorg2 specific files
COPY ./microkorg2/environment /app/microkorg2/
COPY ./builder/get_microkorg2_toolchain.sh /app/microkorg2/
RUN /bin/bash -c "/app/microkorg2/get_microkorg2_toolchain.sh && rm /app/microkorg2/get_microkorg2_toolchain.sh"

# Patch /etc/bash.bashrc to get rid of sudo warnings
COPY ./builder/bash.bashrc.patch /root/
RUN /bin/bash -c "patch -s /etc/bash.bashrc < /root/bash.bashrc.patch; rm /root/bash.bashrc.patch"

RUN /bin/bash -c "addgroup --gid 60000 -q user && \
adduser --shell /usr/bin/bash --no-create-home --home /workspace --uid 60000 --gid 60000 --disabled-password --disabled-login --gecos \"\" -q user && \
usermod -p \"\" user && \
usermod -aG sudo user;"

#### Final image stage

FROM scratch
ARG build
ARG version

LABEL jp.co.korg.vendor="KORG Inc." \
      jp.co.korg.imgid=logue-sdk-dev-env \
      build=$build \
      version=$version \
      description="logue SDK development environment"
      
ENV ENV_BUILD=$build \
    ENV_VERSION=$version

COPY --from=build / /

USER user

WORKDIR /workspace

ENTRYPOINT ["/bin/bash", "-il"]

