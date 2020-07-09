// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "open3d/core/op/linalg/Matmul.h"
#include <unordered_map>

namespace open3d {
namespace core {

void Matmul(const Tensor& lhs, const Tensor& rhs, Tensor& dst) {
    // Check devices
    Device device = lhs.GetDevice();
    if (device != rhs.GetDevice()) {
        utility::LogError(
                "Tensor lhs device {} and Tensor rhs device {} mismatch",
                lhs.GetDevice().ToString(), rhs.GetDevice().ToString());
    }

    // Check dtypes
    Dtype dtype = lhs.GetDtype();
    if (dtype != rhs.GetDtype()) {
        utility::LogError(
                "Tensor lhs dtype {} and Tensor rhs dtype {} mismatch",
                DtypeUtil::ToString(lhs.GetDtype()),
                DtypeUtil::ToString(rhs.GetDtype()));
    }
    if (dtype != Dtype::Float32 && dtype != Dtype::Float64) {
        utility::LogError(
                "Only tensors with Float32 or Float64 are supported, but "
                "received {}",
                DtypeUtil::ToString(dtype));
    }

    // Check shapes
    SizeVector lhs_shape = lhs.GetShape();
    SizeVector rhs_shape = rhs.GetShape();

    if (lhs_shape.size() != 2) {
        utility::LogError("Tensor lhs must be 2D, but got {}D",
                          lhs_shape.size());
    }
    if (rhs_shape.size() != 1 && rhs_shape.size() != 2) {
        utility::LogError(
                "Tensor rhs must be 1D (vector) or 2D (matrix), but got {}D",
                rhs_shape.size());
    }
    if (lhs_shape[1] != rhs_shape[0]) {
        utility::LogError(
                "Tensor lhs columns {} mismatch with Tensor rhs rows {}",
                lhs_shape[1], rhs_shape[0]);
    }

    // Dispatch to backends
    int64_t m = lhs_shape[0];
    int64_t k = lhs_shape[1];
    int64_t n = rhs_shape.size() == 2 ? rhs_shape[1] : 1;
    dst = Tensor::Empty({m, n}, dtype, device);

    Tensor lhs_contiguous = lhs.Contiguous();
    Tensor rhs_contiguous = rhs.Contiguous();
    void* lhs_data = lhs_contiguous.GetDataPtr();
    void* rhs_data = rhs_contiguous.GetDataPtr();
    void* dst_data = dst.GetDataPtr();

    if (device.GetType() == Device::DeviceType::CUDA) {
#ifdef BUILD_CUDA_MODULE
        MatmulCUDA(dtype, lhs_data, rhs_data, dst_data, m, k, n);
#else
        utility::LogError("Unimplemented device.");
#endif
    } else {
        MatmulCPU(dtype, lhs_data, rhs_data, dst_data, m, k, n);
    }
};

}  // namespace core
}  // namespace open3d
