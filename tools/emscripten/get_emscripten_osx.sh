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

REPO_NAME="emsdk"
REPO_URL="https://github.com/emscripten-core/emsdk.git"
REPO_COMMIT="a6c8e25a59cfb4f6ba3ffd982a07aad8a1f89115"

EMSDK_VERSION="1.38.45"

if [[ "${OSTYPE}" == "darwin"* ]]; then
    echo ">> Assuming OSX 64 bit platform."
else
    echo ">> This script is meant for OSX."
    popd 2>&1 > /dev/null
    exit 1
fi

# assert_success(fail_msg)
assert_success() {
    [[ $? -eq 0 ]] && return 0
    [[ ! -z "$1" ]] && echo "Error: $1" 
    popd 2>&1 > /dev/null
    return 1
}

GIT=$(which git) || assert_success "dependency not found..." || exit $?

if [[ ! -d "${REPO_NAME}" ]]; then
    echo ">> Cloning ${REPO_URL}..."
    ${GIT} clone "${REPO_URL}" "${REPO_NAME}"
    assert_success "Cloning ${REPO_URL} failed..." || exit $?
fi

if [[ -d "${REPO_NAME}" ]]; then
    pushd ${REPO_NAME} 2>&1 > /dev/null

    # Checkout specific commit
    ${GIT} checkout "${REPO_COMMIT}"
    assert_success "Failed to checkout..." || exit $?

    
    if [[ ! -x "./emsdk" ]]; then
        echo "Error: Could not find/execute emsdk install script..."
        exit 1
    fi

    echo ">> Installing version ${EMSDK_VERSION}..."
    ./emsdk install "${EMSDK_VERSION}"
    assert_success "Failed to install..." || exit $?

    echo ">> Activating version ${EMSDK_VERSION}"
    ./emsdk activate "${EMSDK_VERSION}"
    assert_success "Failed to activate..." || exit $?
    
    popd 2>&1 > /dev/null
fi

