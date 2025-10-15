#!/usr/bin/env bash

#
#    run_interactive.sh
#
#    Convenience script to spin up a container instance of the logue SDK development
#    environment docker image and start an interactive shell
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

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if [ -z "$(command -v realpath)" ]; then
    source ${SCRIPT_DIR}/inc/realpath
fi

IMAGE_VERSION="latest"
IMAGE_NAME_DEFAULT="xiashj/logue-sdk"
IMAGE_NAME_FALLBACK="logue-sdk-dev-env"

if docker image inspect "${IMAGE_NAME_DEFAULT}:${IMAGE_VERSION}" >/dev/null 2>&1; then
    IMAGE_NAME="${IMAGE_NAME_DEFAULT}"
else
    IMAGE_NAME="${IMAGE_NAME_FALLBACK}"
fi
PLATFORM_PATH=$(realpath "${SCRIPT_DIR}/../platform")

usage () {
    echo ""
    echo "Usage: $0 [OPTIONS] [[REPOSITORY]:VERSION]"
    echo ""
    echo " Spin up a container instance of the logue SDK development environment"
    echo " and start an interactive shell."
    echo ""
    echo "Options:"
    echo "     --platform=PATH   specify path to platform directory (default: ${PLATFORM_PATH})"
    echo " -h, --help            display this help"
    echo ""
    echo "Arguments:"
    echo " [REPOSITORY]          specify docker image name tag (default: ${IMAGE_NAME})"
    echo " [VERSION]             specify docker image version tag (default: ${IMAGE_VERSION})"
    echo ""
}

for o in "$@"; do
    case $o in
        --platform=*)
            PLATFORM_PATH=$(realpath "${o#*=}")
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            ## unknown option
            shift
            ;;
        *)
            IMAGE_NAME="${o%:*}"
            v="${o#*:}"
            if [ "${v}" != "${IMAGE_NAME}" ] && [ ! -z "${v}" ]; then
                IMAGE_VERSION=${v}
            fi
            break
            ;;
    esac
done

if [ ! -d "${PLATFORM_PATH}" ]; then
    echo "[Err] Could not find platform directory at '${PLATFORM_PATH}'" 1>&2
    exit 1
fi

docker run --rm -v "${PLATFORM_PATH}:/workspace" -h logue-sdk -it ${IMAGE_NAME}:${IMAGE_VERSION} /app/interactive_entry

