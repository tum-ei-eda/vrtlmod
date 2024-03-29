####################################################################################################
# Copyright 2022 Chair of EDA, Technical University of Munich
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################################

cmake_minimum_required(VERSION 3.15)
include(FetchContent)

FetchContent_Declare(pugixml_dep
	GIT_REPOSITORY https://github.com/zeux/pugixml.git
	GIT_TAG v1.11.4
)
if(NOT pugixml_dep_POPULATED)
	FetchContent_Populate(pugixml_dep)
	FetchContent_GetProperties(pugixml_dep)
endif()
add_subdirectory(${pugixml_dep_SOURCE_DIR} ${pugixml_dep_BINARY_DIR} EXCLUDE_FROM_ALL)
