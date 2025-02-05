#  Copyright (C) 2021, Christoph Peters, Karlsruhe Institute of Technology
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.


import numpy as np
import os
import imageio
from subprocess import Popen
import re
from time import sleep


def linear_to_srgb(linear):
    """
    Converts all scalars in the given array (which should be normalized to the
    range from zero to one) from a linear scale to the sRGB scale.
    """
    linear = np.maximum(0.0, linear)
    return np.where(linear <= 0.0031308, 12.92 * linear, 1.055 * linear ** (1.0/2.4) - 0.055)


def complete_materials(directory, material_dict):
    """
    Looks for material textures in a directory. If a set does not consist of
    base color, specular and normal, the missing textures are created using
    constant default values or values from the given dictionary.
    :param directory: The path to the directory to look in.
    :param material_dict: A dictionary mapping material names to triples
        (base_color, normal, specular). Each entry can be either None to
        indicate that the global default should be used, a triple of floats
        between 0.0 and 1.0 (-1.0 and 1.0 for normals) that will be converted
        to LDR values in the appropriate way or a triple of integers between
        0 and 255. Either way, it defines what goes into the texture. If the
        dictionary holds material names that do not exist in the directory,
        new materials are created. It is legal to omit the normal entry. If
        specular is just a float, it is interpreted as roughness of a
        dielectric. If base_color is just a float, it is interpreted as grey.
    """
    suffixes = ["_BaseColor", "_Normal", "_Specular"]
    global_defaults = ((0, 0, 0), (128, 128, 255), (255, 128, 0))
    # Unify the meaning of dictionary entries to LDR values
    full_materials = list()
    for name, defaults in material_dict.items():
        if len(defaults) == 2:
            defaults = (defaults[0], None, defaults[1])
        defaults = [global_defaults[i] if default is None else default for i, default in enumerate(defaults)]
        base_color, normal, specular = defaults
        if isinstance(base_color, float):
            base_color = (base_color, base_color, base_color)
        if isinstance(base_color[0], float):
            base_color = tuple(np.asarray(np.round(linear_to_srgb(base_color) * 255.0), dtype=np.uint8))
        if isinstance(normal[0], float):
            normal = tuple(np.asarray(np.round((np.asarray(normal) * 0.5 + 0.5) * 255.0), dtype=np.uint8))
        if isinstance(specular, float):
            specular = (1.0, specular, 0.0)
        if isinstance(specular[0], float):
            specular = tuple(np.asarray(np.round(np.asarray(specular) * 255.0), dtype=np.uint8))
        full_materials.append((name, (base_color, normal, specular)))
    # Add entries for (potentially incomplete) materials
    file_list = os.listdir(directory)
    for file in file_list:
        match = re.search(r"((_BaseColor\.)|(_Normal\.)|(_Specular\.))", file)
        if match is not None and os.path.splitext(file)[1] != ".vkt":
            prefix = file[0:match.start(1)]
            if prefix not in material_dict:
                full_materials.append((prefix, global_defaults))
    # Create the missing texture files
    file_list_no_extension = frozenset([os.path.splitext(file)[0] for file in file_list])
    for name, defaults in full_materials:
        for suffix, default in zip(suffixes, defaults):
            texture_name = name + suffix
            if texture_name not in file_list_no_extension:
                texture_path = os.path.join(directory, texture_name + ".png")
                image = np.asarray(default, dtype=np.uint8)[np.newaxis, np.newaxis, :]
                image = image.repeat(4, 0).repeat(4, 1)
                imageio.imsave(texture_path, image)
                print("Created %s." % texture_path)


def convert_materials(destination_directory, source_directory, skip_existing=True, texture_conversion_path="texture_conversion/build/texture_conversion"):
    """
    This function performs batch conversion of textures from a common file
    format (anything supported by stb_image) into the file format of the
    renderer, which has precomputed mipmaps and block compression.
    :param destination_directory: Texture files with identical name but file
        format extension .vkt are written to this directory. Gets created if it
        does not exist.
    :param source_directory: The directory that is searched for textures. Only
        file names ending with _BaseColor, _Normal or _Specular are considered.
    :param skip_existing: Pass True to skip files that would require the output
        file to be overwritten. Otherwise, output files are overwritten without
        prompt.
    :param texture_conversion_path: Path to a binary of the program in the
        texture_conversion folder.
    """
    if not os.path.exists(destination_directory):
        os.makedirs(destination_directory)
    # Lists (terminated, source_file, args, process). First source_file and
    # args are set, once work starts process is set, once it is finished,
    # terminated is set to True
    tasks = list()
    for file in os.listdir(source_directory):
        match = re.search(r"(_BaseColor\.)|(_Normal\.)|(_Specular\.)", file)
        if match is not None and os.path.splitext(file)[1] != ".vkt":
            source_file = os.path.join(source_directory, file)
            is_srgb = match.group(1) is not None
            is_normal_map = match.group(2) is not None
            destination_file = os.path.join(destination_directory, os.path.splitext(file)[0] + ".vkt")
            if not skip_existing or not os.path.exists(destination_file):
                if is_srgb:
                    format = 132
                elif is_normal_map:
                    format = 141
                else:
                    format = 131
                args = [texture_conversion_path, str(format), source_file, destination_file]
                tasks.append([False, source_file, args, None])
    # Launch all processes without launching too many at once to avoid running
    # out of memory
    cpu_count = os.cpu_count()
    print("Using %d threads to convert materials." % cpu_count)
    if cpu_count is None:
        cpu_count = 32
    launched_count = terminated_count = 0
    while terminated_count < len(tasks):
        for task in tasks:
            terminated, source_file, args, process = task
            if not terminated and process is not None and (return_code := process.poll()) is not None:
                task[0] = True
                terminated_count += 1
                if return_code == 0:
                    print("Finished %s" % source_file)
                else:
                    print("Failed for %s with return code %d." % (source_file, return_code))
            if process is None and launched_count - terminated_count < cpu_count:
                task[3] = Popen(args)
                launched_count += 1
        sleep(0.1)
