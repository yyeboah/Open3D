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

#include "open3d/core/SizeVector.h"
#include "open3d/utility/Console.h"
#include "tests/UnitTest.h"

namespace open3d {
namespace core {

class Densor {
public:
    Densor(std::initializer_list<float> l) {
        values_.insert(values_.end(), l.begin(), l.end());
        shape_ = SizeVector{static_cast<int64_t>(values_.size())};
    }

    virtual ~Densor() {}

    void Print() const {
        utility::LogInfo("[Densor]\nValues: {}\nShape: {}", values_,
                         shape_.ToString());
    }

public:
    std::vector<float> values_;  // Flattended values.
    SizeVector shape_;
};

}  // namespace core
}  // namespace open3d

namespace open3d {
namespace tests {

TEST(Densor, ConstructorOneNested) {
    core::Densor d{0, 1, 2, 3, 4};
    d.Print();
}

}  // namespace tests
}  // namespace open3d
