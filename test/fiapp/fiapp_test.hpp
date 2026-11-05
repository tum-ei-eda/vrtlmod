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

#pragma once

#include "testinject.hpp"

// Require every state shape in fiapp, so partial discovery cannot pass by testing
// only the targets that happened to survive a change in Verilator's generated code.
inline bool check_fiapp_target_coverage(const vrtlfi::td::TD_API &api, std::ostream &out = std::cout)
{
    struct ExpectedTarget
    {
        const char *name;
        unsigned bits;
        unsigned element_bits;
        unsigned instances;
    };
    const ExpectedTarget expected[] = {
        { "q1", 1, 1, 1 },
        { "r_q", 1, 1, 2 },
        { "reg_3_1_VlWide_q", 65, 65, 1 },
        { "reg_3_0_VlWide_q", 96, 96, 1 },
        { "reg_VlUnpacked_CData_q", 6, 3, 1 },
        { "reg_VlUnpacked_VlUnpacked_CData_q", 12, 3, 1 },
        { "reg_VlUnpacked_VlWide_q", 130, 65, 1 },
        { "reg_VlUnpacked_VlUnpacked_VlWide_q", 260, 65, 1 },
    };
    bool passed = true;
    for (const auto &target : expected)
    {
        unsigned found = 0;
        unsigned total_bits = 0;
        for (const auto &[key, entry] : api.td_)
        {
            // Optimized output flattens hierarchy into __DOT__ field names;
            // unoptimized output retains cells and may add a __PVT__ prefix.
            std::string name = key.substr(key.find_last_of('.') + 1);
            const auto dot = name.rfind("__DOT__");
            if (dot != std::string::npos)
                name.erase(0, dot + 7);
            if (name.compare(0, 7, "__PVT__") == 0)
                name.erase(0, 7);
            // Recent Verilator versions split some unpacked arrays into fields.
            const auto index = name.find("__BRA__");
            if (index != std::string::npos)
                name.erase(index);
            if (!entry->injectable_ || name != target.name)
                continue;

            ++found;
            total_bits += entry->bits_;
            if (entry->bits_ > target.bits || entry->bits_ % target.element_bits != 0
                || entry->onedimbits_ != target.element_bits)
            {
                out << "Wrong target dimensions for " << key << ": " << entry->bits_
                    << " bits, " << entry->onedimbits_ << " bits per element\n";
                passed = false;
            }
        }
        if (total_bits != target.bits * target.instances
            || (target.bits == target.element_bits && found != target.instances))
        {
            out << "Expected " << target.bits * target.instances << " injectable bits of " << target.name
                << ", found " << total_bits << " across " << found << " target(s)\n";
            passed = false;
        }
    }
    return passed;
}
