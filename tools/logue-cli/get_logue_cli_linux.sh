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

PKGNAME="logue-cli"
VERSION="0.07-2b"

ARCHIVE_URL="http://cdn.storage.korg.com/korg_SDK/logue-cli-linux64-0.07-2b.tar.gz"
ARCHIVE_SHA1="50af8cad47635f26c8a5c35dcb20f3bbad4f2f2d"
ARCHIVE_NAME="logue-cli-linux64-0.07-2b.tar.gz"

if [[ "${OSTYPE}" == "linux-gnu"* ]]; then
    echo ">> Assuming Linux 64 bit platform."
else
    echo ">> This script is meant for Linux."
    popd 2>&1 > /dev/null
    exit 1
fi

# assert_command(name, exec_path)
assert_command() {
    [[ ! -x "$2" ]] && echo "Error: Could not find '$1' command at path '$2'"
    echo "$2"
}

# assert_success(fail_msg)
assert_success() {
    [[ $? -eq 0 ]] && return 0
    [[ ! -z "$1" ]] && echo "Error: $1" 
    popd 2>&1 > /dev/null
    return 1
}

AWK=$(which awk) || assert_success "dependency not found..." || exit

CURL=$(which curl) || assert_success "dependency not found..." || exit

TAR=$(which tar) || assert_success "dependency not found..." || exit $?

SHA1SUM=$(which sha1sum) || assert_success "dependency not found..." || exit

# test_sha1sum(sha1, path_to_file)
test_sha1sum() {
    SHA1=$(${SHA1SUM} "$2" | ${AWK} '{print $1};')
    [[ "${SHA1}" != "$1" ]] && return 1
    return 0
}

if [[ ! -f "${ARCHIVE_NAME}" ]]; then
    echo ">> Downloading..."
    ${CURL} -# -L -o "${ARCHIVE_NAME}" "${ARCHIVE_URL}"
    assert_success "Download of ${ARCHIVE_NAME} failed..." || exit $?
fi

test_sha1sum "${ARCHIVE_SHA1}" "${ARCHIVE_NAME}"
assert_success "SHA1 mismatch. Try redownloading the archive..." || exit $?

echo ">> Unpacking..."
${TAR} -zxvf "${ARCHIVE_NAME}"
assert_success "Could not unpack archive..." || exit $?

echo ">> Cleaning up..."
rm -f "${ARCHIVE_NAME}"
assert_success "Could not delete temporary archive..." || exit $?

popd 2>&1 > /dev/null

