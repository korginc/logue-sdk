#!/bin/bash
#
# BSD 3-Clause License
#
# Copyright (c) 2018, KORG INC.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SCRIPT_DIR="$(pwd)/$(dirname $0)"

pushd ${SCRIPT_DIR} 2>&1 > /dev/null

# assert_success(fail_msg)
assert_success() {
    [ $? -eq 0 ] && return 0
    [ ! -z "$1" ] && echo "[Err] $1" 1>&2 
    popd 2>&1 > /dev/null
    exit 1
}

CUR_ARCH="$(uname -p)"

if [ "${CUR_ARCH}" == "x86_64" ]; then
    ARCHIVE_URL="https://www.dropbox.com/s/3curhjx9yip29os/drumlogue-toolchain-amd64.tar.bz2"
    ARCHIVE_SHA1="6b2a403195fbac0f7ee8d53bea4ed17b8c14e541"
elif [ "${CUR_ARCH}" == "aarch64" ]; then
    ARCHIVE_URL="https://www.dropbox.com/s/ioysa018ruopp72/drumlogue-toolchain-aarch64.tar.bz2"
    ARCHIVE_SHA1="d907b640967e8ea40baa82ff55d8813716028aa9"
else
    echo "[Err] Unsupported architecture: ${CUR_ARCH}" 1>&2
    exit 1
fi

ARCHIVE_NAME="$(basename ${ARCHIVE_URL})"

# test_sha1sum(sha1, path_to_file)
test_sha1sum() {
    local sum=$(sha1sum "$2")
    [ "${sum%  *}" != "$1" ] && return 1
    return 0
}

if [ ! -f "${ARCHIVE_NAME}" ]; then
    echo ">> Downloading..."
    curl -# -L -o "${ARCHIVE_NAME}" "${ARCHIVE_URL}"
    assert_success "Download of ${ARCHIVE_NAME} failed..."
fi

test_sha1sum "${ARCHIVE_SHA1}" "${ARCHIVE_NAME}"
assert_success "SHA1 mismatch. Try redownloading the archive..."

echo ">> Unpacking..."
UNPACKED_DIR=$(tar -jxvf "${ARCHIVE_NAME}" | cut -f1 -d"/" | sort | uniq)
assert_success "Could not unpack archive..."

echo ">> Staging..."
if [ -z "${UNPACKED_DIR}" ] || [ ! -d "${UNPACKED_DIR}" ]; then
    echo "[Err] Cannot find unpacked directory at: '${SCRIPT_DIR}/${UNPACKED_DIR}'" 1>&2
    exit 1
fi
mv "${UNPACKED_DIR}"/* .
assert_success "Could not stage archive content..."

echo ">> Cleaning up..."
rm -rf "${UNPACKED_DIR}"
rm -f "${ARCHIVE_NAME}"
assert_success "Could not delete intermediate files..."

popd 2>&1 > /dev/null

