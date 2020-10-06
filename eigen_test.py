import open3d as o3d
import numpy as np
import time
import pytest


def test_Vector3dVector(input_array, expect_exception):

    def run_test(input_array):
        open3d_array = o3d.utility.Vector3dVector(input_array)
        output_array = np.asarray(open3d_array)
        np.testing.assert_allclose(input_array, output_array)

    if expect_exception:
        with pytest.raises(Exception):
            run_test(input_array)
    else:
        run_test(input_array)


if __name__ == '__main__':
    input_array = np.ones((2, 4), dtype=np.float64)
    expect_exception = True
    test_Vector3dVector(input_array, expect_exception)
    print("done test")
