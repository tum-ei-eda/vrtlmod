####################################################################################################
# Copyright 2022 Chair of EDA, Technical University of Munich
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################################

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
include(FetchContent)

set(RTL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(RTL_DIR ${RTL_DIR} PARENT_SCOPE)
set(TOP_NAME foo PARENT_SCOPE)
message("RTL_DIR: ${RTL_DIR}")

set(RTL_SRC_FILES
    ${RTL_DIR}/foo.v
    ${RTL_DIR}/dff.v
)

set(RTL_SRCS ${RTL_SRC_FILES}
    PARENT_SCOPE
)

set(RTL_INCLUDE_DIRS
    ${RTL_DIR}
    PARENT_SCOPE
)

set(VERILATE_ARGS
    -sv
    +define+WT_DCACHE
    -O3
    --unroll-count 256
    --pins-bv 2
    -Werror-PINMISSING
    -Wno-fatal
    -Wno-PINCONNECTEMPTY
    -Wno-ASSIGNDLY
    -Wno-DECLFILENAME
    -Wno-UNUSED
    -Wno-UNOPTFLAT
    -Wno-style
    -Wno-CASEINCOMPLETE
    -Wno-LITENDIAN
    -Wno-UNOPT
    -Wno-WIDTH
    -Wno-BLKANDNBLK
    PARENT_SCOPE
)
