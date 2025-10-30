import re
import struct
import numpy as np
from math import prod
import openvdb as vdb
from os import path


def load_vdb(file_path, grid_name):
    """
    Loads the scalar-valued grid with the given name from the given OpenVDB
    file and returns it as NumPy array.
    """
    grid = vdb.read(file_path, grid_name)
    grid_min, grid_max = grid.evalActiveVoxelBoundingBox()
    array = np.zeros(np.asarray(grid_max) - np.asarray(grid_min), dtype=np.float32)
    grid.copyToArray(array, ijk=grid_min)
    if not grid.transform.isLinear:
        raise TypeError("Only VDB files with linear transforms are supported.")
    origin, x, y, z = [grid.transform.indexToWorld(index) for index in [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]]
    texel_to_world_space = np.vstack([x, y, z, origin]).T
    texel_to_world_space[:, :3] -= texel_to_world_space[:, 3:]
    return array, texel_to_world_space


def write_volume(file_path, volume, texel_to_world_space, vulkan_format=100):
    """
    Writes a volume to a simple binary file format. See the comments in the
    implementation for documentation.
    """
    if texel_to_world_space.shape != (3, 4):
        raise ValueError("The shape of a texel to world space transform for a volume must be 3x4 but got %s." % str(texel_to_world_space.shape))
    supported_formats = [76, 100]
    if vulkan_format not in supported_formats:
        raise ValueError("Vulkan format %d is not supported. Only the following formats are supported: %s" % (vulkan_format, str(supported_formats)))
    with open(file_path, "wb") as file:
        # File format marker (ASCII for 'volume')
        file.write(struct.pack("Q", 0x656d756c6f76))
        # File format version
        file.write(struct.pack("Q", 0))
        # Grid extent
        file.write(struct.pack("QQQ", *volume.shape))
        # A VkFormat indicating the quantization of volume values
        file.write(struct.pack("I", vulkan_format))
        # A float[3][4] providing the texel to world space transform. In texel
        # space, voxel boundaries have integer coordinates and the first voxel
        # in the array corresponds to the unit cube.
        transform = np.asarray(texel_to_world_space, dtype=np.float32, order="C")
        file.write(transform.data)
        # Consecutive voxel values in an order where the index triples are
        # sorted lexicographically. For block-compressed formats, these are
        # consecutive blocks.
        dtype = {76: np.float16, 100: np.float32}[vulkan_format]
        print(file_path, np.shape(volume), np.min(volume), np.max(volume))
        transposed_volume = np.asarray(volume.T, dtype=dtype, order="C")
        file.write(transposed_volume.data)


def convert_volume(file_path, value_scale=1.0, world_to_new_world_space=np.eye(4)):
    """
    Creates a *.blob file next to the given volume file. Values are scaled by
    the given factor and the given transform is applied on top of the texel to
    world space transform of the volume.
    """
    fun_dict = { ".vdb": lambda path: load_vdb(path, "density") }
    name, extension = path.splitext(file_path)
    fun = fun_dict[extension.lower()]
    volume, texel_to_world_space = fun(file_path)
    texel_to_new_world_space = world_to_new_world_space @ np.vstack([texel_to_world_space, np.eye(4)[3:]])
    volume *= value_scale
    write_volume(name + ".blob", volume, texel_to_new_world_space[:3], 76)


if __name__ == "__main__":
    transform = np.eye(4)[:3, :]
    transform[1, 1] = transform[2, 2] = 0.0
    transform[1, 2] = -1.0
    transform[2, 1] = 1.0
    convert_volume("/home/cpeters/data/volumes/IntelVolumetricClouds_full/intelCloudLib_dense.4.M.vdb", 1.0, transform)
    convert_volume("/home/cpeters/data/volumes/wdas_cloud/wdas_cloud_half.vdb", 1.0, transform)
    convert_volume("/home/cpeters/data/volumes/bunny_cloud/bunny_cloud.vdb", 1.0, transform)
    convert_volume("/home/cpeters/data/volumes/explosion_vdb/explosion.vdb", 1.0, transform)
    convert_volume("/home/cpeters/data/volumes/smoke/smoke.vdb", 1.0, transform)
    convert_volume("/home/cpeters/data/volumes/smoke2/smoke2.vdb", 1.0, transform)
    convert_volume("/home/cpeters/data/volumes/fire/fire.vdb", 1.0, transform)
