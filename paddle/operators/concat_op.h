/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include <vector>
#include "paddle/framework/op_registry.h"
#include "paddle/operators/strided_memcpy.h"

namespace paddle {
namespace operators {

template <typename DeviceContext, typename T>
class ConcatKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext& ctx) const override {
    auto ins = ctx.MultiInput<framework::Tensor>("X");
    auto* out = ctx.Output<framework::Tensor>("Out");
    int64_t axis = static_cast<int64_t>(ctx.Attr<int>("axis"));
    const size_t n = ins.size();
    size_t output_offset = 0;
    out->mutable_data<T>(ctx.GetPlace());
    auto out_stride = framework::stride(out->dims());
    for (size_t i = 0; i < n; i++) {
      auto& in = ins[i];
      auto axis_dim = in->dims()[axis];
      auto in_stride = framework::stride(in->dims());
      StridedMemcpy<T>(ctx.device_context(), in->data<T>(), in_stride,
                       in->dims(), out_stride, out->data<T>() + output_offset);
      output_offset += axis_dim * in_stride[axis];
    }
  }
};

template <typename DeviceContext, typename T>
class ConcatGradKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext& ctx) const {
    auto* in = ctx.Input<framework::Tensor>(framework::GradVarName("Out"));
    auto outs = ctx.MultiOutput<framework::Tensor>(framework::GradVarName("X"));
    int64_t axis = static_cast<int64_t>(ctx.Attr<int>("axis"));
    const size_t n = outs.size();
    size_t input_offset = 0;
    auto in_stride = framework::stride(in->dims());
    for (size_t i = 0; i < n; i++) {
      auto& out = outs[i];
      out->mutable_data<T>(ctx.GetPlace());
      size_t axis_dim = out->dims()[axis];
      auto out_stride = framework::stride(out->dims());
      StridedMemcpy<T>(ctx.device_context(), in->data<T>() + input_offset,
                       in_stride, out->dims(), out_stride, out->data<T>());
      input_offset += axis_dim * in_stride[axis];
    }
  }
};

}  // namespace operators
}  // namespace paddle
