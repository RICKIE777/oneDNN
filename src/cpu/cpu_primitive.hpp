/*******************************************************************************
* Copyright 2019-2022 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef CPU_CPU_PRIMITIVE_HPP
#define CPU_CPU_PRIMITIVE_HPP

#include <assert.h>

#include "oneapi/dnnl/dnnl_types.h"

#include "common/c_types_map.hpp"
#include "common/primitive_attr.hpp"
#include "common/primitive_exec_types.hpp"
#include "common/utils.hpp"
#include "common/z_magic.hpp"

#define DEFINE_SCALES_BUFFER_ATTR(attr, scales) \
    alignas(16) float CONCAT2(scales, _buf16)[16] = {0}; \
    const float *scales {nullptr}; \
    if ((attr)) { \
        if ((attr)->output_scales_.defined()) { \
            scales = (attr)->output_scales_.scales_; \
        } else { \
            scales = CTX_IN_MEM(const float *, DNNL_ARG_ATTR_OUTPUT_SCALES); \
            if (scales == nullptr) return status::invalid_arguments; \
            const auto scales_d = ctx.memory_mdw(DNNL_ARG_ATTR_OUTPUT_SCALES); \
            bool ok = scales_d.data_type() == data_type::f32 \
                    && scales_d.ndims() == 1; \
            if (!ok) return status::invalid_arguments; \
            if (scales_d.dims()[0] == 1) { \
                utils::array_set(CONCAT2(scales, _buf16), scales[0], 16); \
                scales = CONCAT2(scales, _buf16); \
            } \
        } \
    } \
    MAYBE_UNUSED(scales);

#define DEFINE_SCALES_BUFFER(scales) \
    DEFINE_SCALES_BUFFER_ATTR(pd()->attr(), scales)

#define DEFINE_ZERO_POINTS_BUFFER(zero_points_ptr, mem_arg) \
    const int32_t *zero_points_ptr \
            = pd()->attr()->zero_points_.defined(mem_arg) \
            ? pd()->attr()->zero_points_.get(mem_arg) \
            : CTX_IN_MEM( \
                    const int32_t *, DNNL_ARG_ATTR_ZERO_POINTS | mem_arg); \
    if (zero_points_ptr == nullptr) return status::invalid_arguments; \
    MAYBE_UNUSED(zero_points_ptr);

#define DEFINE_INPUT_ZERO_POINTS_BUFFER(input_zero_points_ptr, jcp) \
    const uint8_t *input_zero_points_ptr = nullptr; \
    if (jcp.with_input_zp) { \
        input_zero_points_ptr = CTX_IN_MEM(const uint8_t *, DNNL_ARG_ATTR_ZERO_POINTS | DNNL_ARG_SRC); \
        if (input_zero_points_ptr == nullptr) return status::invalid_arguments; \
    }

#define DEFINE_OUTPUT_COMPENSATION_BUFFER(output_compensation_ptr, jcp) \
    const int32_t *output_compensation_ptr = nullptr; \
    if (jcp.with_input_zp) { \
        output_compensation_ptr = CTX_IN_MEM(const int32_t *, DNNL_ARG_ATTR_ZERO_POINTS | DNNL_ARG_DST); \
        if (output_compensation_ptr == nullptr) return status::invalid_arguments; \
    }

#define ASSIGN_INPUT_SCALE_VALUE(scale, mem_arg) \
    if (pd()->attr()->scales_.get(mem_arg).defined()) { \
        scale = pd()->attr()->scales_.get(mem_arg).scales_; \
    } else { \
        const auto scale_d \
                = ctx.memory_mdw(DNNL_ARG_ATTR_INPUT_SCALES | mem_arg); \
        bool ok = scale_d.data_type() == data_type::f32 \
                && scale_d.ndims() == 1 && scale_d.dims()[0] == 1; \
        if (!ok) return status::invalid_arguments; \
        const float *scale_p = CTX_IN_MEM( \
                const float *, DNNL_ARG_ATTR_INPUT_SCALES | mem_arg); \
        if (scale_p == nullptr) return status::invalid_arguments; \
        scale = scale_p; \
    }

#define DEFINE_ZERO_POINT_VALUE(zero_point, mem_arg) \
    int32_t zero_point = 0; \
    if (pd()->attr()->zero_points_.defined(mem_arg)) { \
        const bool is_common = pd()->attr()->zero_points_.common(mem_arg); \
        assert(is_common && "expect common zero point"); \
        if (!is_common) return status::runtime_error; \
        zero_point = *pd()->attr()->zero_points_.get(mem_arg); \
    } else { \
        const auto zero_points_d \
                = ctx.memory_mdw(DNNL_ARG_ATTR_ZERO_POINTS | mem_arg); \
        bool ok = zero_points_d.data_type() == data_type::s32 \
                && zero_points_d.ndims() == 1 && zero_points_d.dims()[0] == 1; \
        if (!ok) return status::invalid_arguments; \
        const int32_t *zero_points_ptr = CTX_IN_MEM( \
                const int32_t *, DNNL_ARG_ATTR_ZERO_POINTS | mem_arg); \
        if (zero_points_ptr == nullptr) return status::invalid_arguments; \
        zero_point = *zero_points_ptr; \
    } \
    MAYBE_UNUSED(zero_point);

#endif // CPU_CPU_PRIMITIVE_HPP
