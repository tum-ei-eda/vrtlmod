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

#ifndef __COMMON_TRANSACTION_COMMONS_H__
#define __COMMON_TRANSACTION_COMMONS_H__

#include <tlm/scc/tlm_id.h>
#include <tlm/scc/tlm_mm.h>
#include <tlm/scc/tlm_extensions.h>

tlm::tlm_generic_payload *prepare_trans(size_t len, int be_flags = -1)
{
    static int id = 0;
    auto trans = tlm::scc::tlm_mm<tlm::tlm_base_protocol_types, false>::get().allocate<tlm::scc::data_buffer>(len);
    tlm::scc::setId(*trans, id++);
    trans->set_streaming_width(len);

    if (be_flags == -1)
    {
        trans->set_byte_enable_ptr(NULL);
        trans->set_byte_enable_length(0);
    }
    else
    {
        auto be = new unsigned char[len];
        for (int byte_i = 0; byte_i < len; ++byte_i)
        {
            be[byte_i] = (be_flags & 0x1) ? TLM_BYTE_ENABLED : TLM_BYTE_DISABLED;
            be_flags >>= 1;
        }
        trans->set_byte_enable_ptr(be);
        trans->set_byte_enable_length(len);
    }
    return trans;
}

#endif // __COMMON_TRANSACTION_COMMONS_H__
