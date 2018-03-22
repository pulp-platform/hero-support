#! /bin/bash
# Copyright (C) 2018 ETH Zurich and University of Bologna
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

source platforms/hero-zc706-env.sh

mkdir -p ${HERO_SDK_DIR}/hero-gnu-gcc-toolchain
cd ${HERO_SDK_DIR}/hero-gnu-gcc-toolchain
if [ ! -d ${HERO_SDK_DIR}/hero-gnu-gcc-toolchain/.git ]; then
	git init .
	git remote add origin git@iis-git.ee.ethz.ch:alessandro.capotondi/hero-toolchain.git
fi
git pull origin master

# That's all folks!!
