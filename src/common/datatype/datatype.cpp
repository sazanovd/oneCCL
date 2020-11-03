/*
 Copyright 2016-2020 Intel Corporation
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include <limits>

#include "common/datatype/datatype.hpp"
#include "common/global/global.hpp"
#include "common/utils/enums.hpp"
#include "exec/exec.hpp"

ccl_datatype ccl_datatype_char;

ccl::datatype& operator++(ccl::datatype& d) {
    using IntType = typename std::underlying_type<ccl::datatype>::type;
    d = static_cast<ccl::datatype>(static_cast<IntType>(d) + 1);
    return d;
}

ccl::datatype operator++(ccl::datatype& d, int) {
    ccl::datatype tmp(d);
    ++d;
    return tmp;
}

ccl_datatype::ccl_datatype(ccl::datatype idx, size_t size) : m_idx(idx), m_size(size) {
    CCL_THROW_IF_NOT(m_size > 0, "unexpected datatype size ", m_size);
}

ccl_datatype_storage::ccl_datatype_storage() {
    LOG_DEBUG("create datatype_storage");

    using IntType = typename std::underlying_type<ccl::datatype>::type;
    custom_idx = static_cast<ccl::datatype>(static_cast<IntType>(ccl::datatype::last_predefined) + 1);

    size_t size = 0;
    std::string name_str;

    for (ccl::datatype idx = ccl::datatype::int8; idx <= ccl::datatype::last_predefined; idx++) {
        /* fill table with predefined datatypes */
        size = (idx == ccl::datatype::int8)       ? sizeof(char)
               : (idx == ccl::datatype::uint8)    ? sizeof(uint8_t)
               : (idx == ccl::datatype::int16)    ? sizeof(int16_t)
               : (idx == ccl::datatype::uint16)   ? sizeof(uint16_t)
               : (idx == ccl::datatype::int32)    ? sizeof(int32_t)
               : (idx == ccl::datatype::uint32)   ? sizeof(uint32_t)
               : (idx == ccl::datatype::int64)    ? sizeof(int64_t)
               : (idx == ccl::datatype::uint64)   ? sizeof(uint64_t)
               : (idx == ccl::datatype::float16)  ? sizeof(uint16_t)
               : (idx == ccl::datatype::float32)  ? sizeof(float)
               : (idx == ccl::datatype::float64)  ? sizeof(double)
               : (idx == ccl::datatype::bfloat16) ? sizeof(uint16_t)
                                                  : 0;

        CCL_ASSERT(size > 0, "Unexpected data type size: ", size, ", for idx: ", idx);
        name_str = (idx == ccl::datatype::int8)       ? "INT8"
                   : (idx == ccl::datatype::uint8)    ? "UINT8"
                   : (idx == ccl::datatype::int16)    ? "INT16"
                   : (idx == ccl::datatype::uint16)   ? "UINT16"
                   : (idx == ccl::datatype::int32)    ? "INT32"
                   : (idx == ccl::datatype::uint32)   ? "UINT32"
                   : (idx == ccl::datatype::int64)    ? "INT64"
                   : (idx == ccl::datatype::uint64)   ? "UINT64"
                   : (idx == ccl::datatype::float16)  ? "FLOAT16"
                   : (idx == ccl::datatype::float32)  ? "FLOAT"
                   : (idx == ccl::datatype::float64)  ? "DOUBLE"
                   : (idx == ccl::datatype::bfloat16) ? "BFLOAT16"
                                                      : 0;

        create_internal(predefined_table, idx, size, name_str);

        const ccl_datatype& dtype = get(idx);
        const std::string& dtype_name = name(dtype);

        CCL_THROW_IF_NOT(
            dtype.idx() == idx, "unexpected datatype idx ", dtype.idx(), ", expected ", idx);
        CCL_THROW_IF_NOT(
            dtype.idx() == idx, "unexpected datatype size ", dtype.size(), ", expected ", size);
        CCL_THROW_IF_NOT(!dtype_name.compare(name_str),
                         "unexpected datatype name ",
                         dtype_name,
                         ", expected ",
                         name_str);
    }

    ccl_datatype_char = get(ccl::datatype::int8);
}

ccl_datatype_storage::~ccl_datatype_storage() {
    std::lock_guard<ccl_datatype_lock_t> lock{ guard };
    predefined_table.clear();
    custom_table.clear();
}

void ccl_datatype_storage::create_internal(ccl_datatype_table_t& table,
                                           ccl::datatype idx,
                                           size_t size,
                                           const std::string& name) {
    CCL_THROW_IF_NOT(table.find(idx) == table.end(), "datatype index is busy, idx ", idx);
    table[idx] = std::make_pair(ccl_datatype(idx, size), name);
    LOG_DEBUG("created datatype idx: ", idx, ", size: ", size, ", name: ", name);
}

ccl::datatype ccl_datatype_storage::create_by_datatype_size(size_t datatype_size) {
    std::lock_guard<ccl_datatype_lock_t> lock{ guard };

    while (custom_table.find(custom_idx) != custom_table.end() ||
           is_predefined_datatype(custom_idx)) {
        custom_idx++;
        if (custom_idx < ccl::datatype::int8)
            custom_idx = ccl::datatype::int8;
    }

    CCL_ASSERT(datatype_size > 0);
    create_internal(custom_table,
                    custom_idx,
                    datatype_size,
                    std::string("DTYPE_") + std::to_string(utils::enum_to_underlying(custom_idx)));

    return custom_idx;
}

ccl::datatype ccl_datatype_storage::create(const ccl::datatype_attr& attr) {
    size_t size = attr.get<ccl::datatype_attr_id::size>();
    return create_by_datatype_size(size);
}

void ccl_datatype_storage::free(ccl::datatype idx) {
    std::lock_guard<ccl_datatype_lock_t> lock{ guard };

    if (is_predefined_datatype(idx)) {
        CCL_THROW("attempt to free predefined datatype idx ", idx);
        return;
    }

    if (custom_table.find(idx) == custom_table.end()) {
        CCL_THROW("attempt to free non-existing datatype idx ", idx);
        return;
    }

    LOG_DEBUG("free datatype idx ", idx);
    custom_table.erase(idx);
}

const ccl_datatype& ccl_datatype_storage::get(ccl::datatype idx) const {
    if (is_predefined_datatype(idx)) {
        return predefined_table.find(idx)->second.first;
    }
    else {
        std::lock_guard<ccl_datatype_lock_t> lock{ guard };
        return custom_table.find(idx)->second.first;
    }
}

const std::string& ccl_datatype_storage::name(const ccl_datatype& dtype) const {
    ccl::datatype idx = dtype.idx();
    if (is_predefined_datatype(idx)) {
        return predefined_table.find(idx)->second.second;
    }
    else {
        std::lock_guard<ccl_datatype_lock_t> lock{ guard };
        return custom_table.find(idx)->second.second;
    }
}

const std::string& ccl_datatype_storage::name(ccl::datatype idx) const {
    return name(get(idx));
}

bool ccl_datatype_storage::is_predefined_datatype(ccl::datatype idx) {
    return (idx >= ccl::datatype::int8 && idx <= ccl::datatype::last_predefined) ? true : false;
}
