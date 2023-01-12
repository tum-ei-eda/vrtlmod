/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

////////////////////////////////////////////////////////////////////////////////
/// @file testinject.hpp
/// @date 2022-23-12
////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <iostream>

#include "targetdictionary.hpp"

bool testinject(vrtlfi::td::TDentry &target, vrtlfi::td::TD_API &api, std::function<void(int)> const &clockspin,
                std::function<void(void)> const &reset,
                std::function<int(vrtlfi::td::TDentry const *)> const &check_diff, std::ostream &out = std::cout);
