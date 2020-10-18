import open3d as o3d
import time
import numpy as np

np.random.seed(0)
pcd = o3d.geometry.PointCloud()
pcd.points = o3d.utility.Vector3dVector(np.random.rand(int(1e8), 3))
box = o3d.geometry.AxisAlignedBoundingBox((0.25, 0.25, 0.25),
                                          (0.75, 0.75, 0.75))

s = time.time()
indices = box.get_point_indices_within_bounding_box(pcd.points)
print(len(indices))
print("without tbb", time.time() - s)

s = time.time()
indices_tbb = box.get_point_indices_within_bounding_box_tbb(pcd.points)
print(len(indices_tbb))
print("with tbb", time.time() - s)
