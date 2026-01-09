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
    echo "     --platform=PATH    specify path to platform directory (default: ${PLATFORM_PATH})"
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

# Normalize path for Docker Desktop on Windows
# Docker Desktop with WSL2 backend needs /mnt/e/ style paths
PLATFORM_MOUNT="${PLATFORM_PATH}"
UNAME_S=$(uname -s 2>/dev/null || echo "")

if [[ "${OSTYPE}" == msys* || "${OSTYPE}" == cygwin* || "${UNAME_S}" =~ MINGW ]]; then
    # Running in Git Bash/MSYS2 on Windows with Docker Desktop WSL2 backend
    if [[ "${PLATFORM_PATH}" =~ ^/([a-z])/ ]]; then
        # Convert /e/path to /mnt/e/path for Docker WSL2 backend
        DRIVE_LETTER=$(echo "${PLATFORM_PATH:1:1}" | tr '[:lower:]' '[:lower:]')
        REST_PATH="${PLATFORM_PATH:2}"
        PLATFORM_MOUNT="/mnt/${DRIVE_LETTER}${REST_PATH}"
    fi
elif [[ "${PLATFORM_PATH}" =~ ^([A-Za-z]): ]]; then
    # Running from PowerShell/CMD - convert E:\path to /mnt/e/path
    DRIVE_LETTER=$(echo "${PLATFORM_PATH:0:1}" | tr '[:upper:]' '[:lower:]')
    REST_PATH=$(echo "${PLATFORM_PATH:2}" | sed 's|\\|/|g')
    PLATFORM_MOUNT="/mnt/${DRIVE_LETTER}${REST_PATH}"
fi

echo "[Info] Platform path: ${PLATFORM_PATH}"
echo "[Info] Mount path: ${PLATFORM_MOUNT}"
echo "[Info] Docker command:"
echo "[Info] docker run --rm -v \"${PLATFORM_MOUNT}:/workspace\" -h logue-sdk -it ${IMAGE_NAME}:${IMAGE_VERSION} /app/interactive_entry"
echo "[Info] If the mount is empty in the container, ensure Docker Desktop has"
echo "[Info] file sharing enabled for the drive in Settings > Resources > File Sharing"
echo ""
echo "[Info] If you encounter permission errors, run these commands once in the container:"
echo "       sudo mkdir -p ~/.drumlogue.env_backup"
echo "       sudo chown -R \$USER:\$USER ~/.drumlogue.env_backup"
echo ""

# Disable MSYS path conversion for Git Bash on Windows
# This prevents /app/interactive_entry from being converted to C:/Program Files/Git/app/...
export MSYS_NO_PATHCONV=1

docker run --rm -v "${PLATFORM_MOUNT}:/workspace" -h logue-sdk -it ${IMAGE_NAME}:${IMAGE_VERSION} /app/interactive_entry

