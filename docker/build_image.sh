#!/usr/bin/env bash

#
#    build_image.sh - Convenience script to create the logue SDK development environment docker image
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

APP_DIR=${SCRIPT_DIR}/docker-app

IMAGE_NAME="logue-sdk-dev-env"
VERSION="$(cat ${APP_DIR}/VERSION)"
BUILD_ID="$(git rev-parse --short HEAD)"

usage () {
    echo ""
    echo "Usage: $0 [OPTIONS] [[REPOSITORY]:VERSION]"
    echo ""
    echo " Create a docker image for the logue SDK development environment."
    echo ""
    echo "Options:"
    echo "   , --no-pruning      do not prune intermediate images after build"
    echo " -h, --help            display this help"
    echo ""
    echo "Arguments:"
    echo " [REPOSITORY]          specify docker image name tag (default: ${IMAGE_NAME})"
    echo " [VERSION]             specify docker image version tag (default: ${VERSION})"
    echo ""
}

for o in "$@"; do
    case $o in
        --no-pruning)
            OPT_NO_PRUNING="yes"
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
                VERSION=${v}
            fi
            break
            ;;
    esac
done

# Build actual image
docker build --build-arg build=${BUILD_ID} --build-arg version=${VERSION} -t ${IMAGE_NAME}:${VERSION} -t ${IMAGE_NAME}:latest "${APP_DIR}"
res=$?
if [ $res -ne 0 ]; then
    echo "Error: Failed to build docker image (${res})"
    exit $res
fi

# Cleanup build stage images, leaving only the flattened final image
if [ -z "${OPT_NO_PRUNING}" ]; then
   docker image prune -f -a --filter label=jp.co.korg.imgid=logue-sdk-dev-env --filter label=stage=builder --filter label=build=${BUILD_ID}
fi

