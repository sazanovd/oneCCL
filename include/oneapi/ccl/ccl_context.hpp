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
#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

class ccl_context_impl;
namespace ccl {

/**
 * A context object is an abstraction over CPU/GPU context
 * Has no defined public constructor. Use ccl::environment::create_context
 * for context objects creation
 */
/**
 * Stream class
 */
class context : public ccl_api_base_copyable<context, direct_access_policy, ccl_context_impl> {
public:
    using base_t = ccl_api_base_copyable<context, direct_access_policy, ccl_context_impl>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    /**
     * Declare native context type
     */
    using native_t = typename details::ccl_api_type_attr_traits<ccl::context_attr_id,
                                                                ccl::context_attr_id::native_handle>::return_type;
    context(context&& src);
    context(const context& src);
    context& operator=(const context& src);
    context& operator=(context&& src);
    ~context();

    /**
     * Get specific attribute value by @attrId
     */
    template <context_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<context_attr_id, attrId>::return_type& get()
        const;

    /**
     * Get native context object
     */
     native_t& get_native();
     const native_t& get_native() const;
private:
    friend class environment;
    friend class communicator;
    friend class device_context_communicator;
    context(impl_value_t&& impl);

    /**
     *Parametrized context creation helper
     */
    template <context_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    typename ccl::details::ccl_api_type_attr_traits<ccl::context_attr_id, attrId>::return_type set(const Value& v);

    void build_from_params();
    context(const typename details::ccl_api_type_attr_traits<context_attr_id,
                                                           context_attr_id::version>::type& version);

    /**
     * Factory methods
     */
    template <class device_context_type,
              class = typename std::enable_if<is_context_supported<device_context_type>()>::type>
    static context create_context(device_context_type&& native_device_context);

    template <class device_context_handle_type, class... attr_value_pair_t>
    static context create_context_from_attr(device_context_handle_type& native_device_context_handle,
                                        attr_value_pair_t&&... avps);
};

template <context_attr_id t, class value_type>
constexpr auto attr_val(value_type v) -> details::attr_value_tripple<context_attr_id, t, value_type> {
    return details::attr_value_tripple<context_attr_id, t, value_type>(v);
}

} // namespace ccl
