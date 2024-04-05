/*
 * Copyright 2024 Chair of EDA, Technical University of Munich
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

#include "vrtlmod/vapi/generator.hpp"
#include "vrtlmod/core/core.hpp"
#include "vrtlmod/core/types.hpp"
#include "vrtlmod/util/logging.hpp"

#include <boost/algorithm/string/replace.hpp>

namespace vrtlmod
{
namespace vapi
{

std::string VapiGenerator::VapiTargetDictionaryPythonModule::get_filename(void) const
{
    std::string top_name = gen_.get_core().get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4204
    // nothing to do here
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif

    return util::concat(top_name, "_", API_PYTHON_TD_NAME);
}
std::string VapiGenerator::VapiTargetDictionaryPythonModule::generate_body(void) const
{
    int td_nmb = 0;
    bool fast_compare = false;

    const auto &core = gen_.get_core();
    std::string top_name = core.get_top_cell().get_type();
#if VRTLMOD_VERILATOR_VERSION <= 4202
#else // VRTLMOD_VERILATOR_VERSION <= 4228
    util::strhelp::replace(top_name, "___024root", "");
#endif
    std::string top_type = top_name;

    std::stringstream x, entries;

    std::string api_name = top_type + "VRTLmodAPI";

    // TODO: implement this python module
    bool first = true;

    x << R"(class TD:
    """
    Trackable targets, use data_ as: <id>: (<name>, <nmb_bits>, <injectable>)
    """
    data_ = {
          )";

    auto write_data_set = [&](const types::Module &M) -> bool {
        types::Module const *m = &M;
        types::Cell const *c = nullptr;

        auto celliter = [&](const types::Cell &C) -> bool {
            if (m == core.get_module_from_cell(C))
            {
                c = &C;
                return false;
            }
            return true;
        };
        core.foreach_cell(celliter);

        if (c == nullptr)
        {
            LOG_FATAL("Can not find parent Cell of Module ", m->get_id(), " [", m->get_name(), "]");
        }

        auto targetiterf = [&](const types::Target &t) -> bool {
            if (t.get_parent() == *m)
            {
                for (const auto &module_instance : t.get_parent().symboltable_instances_)
                {
                    auto prefix_str = core.get_prefix(c, module_instance);

                    std::string initializer_str = util::concat(
                        // clang-format off
                          "( "
                        , util::concat("\"", prefix_str, ".", t.get_id(), "\"")
                        , ", "
                        , std::to_string(t.get_bits())
                        , ", "
                        , "True"
                        , " )"
                        // clang-format on
                    );
                    if (!first)
                    {
                        x << R"(
        , )";
                    }
                    x << td_nmb++ << ": " << initializer_str;
                    first = false;
                }
            }
            return true;
        };
        core.foreach_injection_target(targetiterf);

        return true;
    };

    td_nmb = 0;
    core.foreach_module(write_data_set);

    if (core.is_systemc())
    {
        if (auto top_module = core.get_module_from_cell(core.get_top_cell()))
        {
            for (auto const &var : top_module->variables_)
            {
                std::string name = var->get_type(); //< seems odd to get name by get_type, but the name is the XML type
                                                    // where id is the XML node type
                if (name == "in" || name == "out" || name == "inout")
                {
                    std::string initializer_str = util::concat(
                        // clang-format off
                          "( "
                        , util::concat("\"", "TOP", ".", var->get_id(), "\"")
                        , ", "
                        , std::to_string(var->get_bits())
                        , " )"
                        // clang-format on
                    );
                    x << R"(
        , )" << td_nmb++
                      << ": " << initializer_str;
                }
            }
        }
    }
    x << R"(
    }

    def get_name_from_id(self, target_id):
        """
        Get the name of a passed target by ID
        """
        name, __, ___ = self.data_[target_id]
        return name

    def get_id_from_name(self, target_name):
        """
        Get the ID for the passed target by name
        """
        for id in self.data_:
            name, __, ___ = self.data_[id]
            if name == target_name: return id
        return None 

    def is_target_injectable(self, target):
        """
        Check whether the passed target (either and ID or a name) is injectable
        """
        if isinstance(target, str):
            target = self.get_id(target)

        _, __, injectable = self.data_[target]
        return injectable;


    def get_target_ubit_map(self):
        """
        Return the target map as a map of single unique bits and their reference to
        their hosting target's (<id>, <name>, <bitoffset within target>)
        """
        ubit = 0
        ret = dict()
        for id in self.data_:
            name, bits, injectable = self.data_[id]
            for bit in range(bits):
                ret[ubit] = (id, name, bit)
                ubit += 1
        return ret

    def get_target_ubit_config(self, ubit, ubit_map=None):
        """
        Use python's wacky mutable default arguments to accel multiple ubit-wise map accesses
        """
        if ubit_map == None:
            ubit_map = self.get_target_ubit_map()
        return ubit_map[ubit]

    def get_target_id_map(self):
        """
        Return the target map as a list of dicts with <id>, <name>, <size>, <ubit_start>, and <ubit_end> keys
        """
        ubit_count = 0
        ret = []
        for id in self.data_:
            name, bits, injectable = self.data_[id]
            ret.append( { 'id': id, 'name': name, 'size': bits, 'ubit_start': ubit_count, 'ubit_end': ubit_count+bits-1 } )
            ubit_count += bits
        return ret

    def get_targets(self):
        """
        !!!DEPRECATED!!! use the TD.data_
        Get the TD's targets as a tuple of their names.
        """
        ret = ()
        for id in self.data_:
            name, __, ___ = self.data_[id]
            ret += (name,)
        return ret

    def get_target_map(self):
        """
        !!!DEPRECATED!!! use the TD.data_
        Get the TD as a map with name-keys and bits as value
        """
        ret = {}
        for id in self.data_:
            name, bits, ___ = self.data_[id]
            ret[name] = bits
        return ret
)";

    return x.str();
}

std::string VapiGenerator::VapiTargetDictionaryPythonModule::generate_header(std::string filename) const
{
    std::stringstream x;
    x << R"("""
@file )"
      << filename << R"(
@date )"
      << get_date() << R"(
@author )"
      << get_author() << R"(
@brief )"
      << get_brief() << R"(
@details )"
      << get_details() << R"(
"""
)";
    return x.str();
}

} // namespace vapi
} // namespace vrtlmod
