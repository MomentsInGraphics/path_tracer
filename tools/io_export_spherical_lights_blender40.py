import bpy
from struct import pack

# To define light sources, create a mesh for the unit sphere named
# "spherical_light" in Blender. Give it a material called _emission. Then place
# suitably scaled instances of this mesh in your scene wherever you want a
# spherical light. All of them have equal brightness. Run this script to create
# the *.lights file and io_export_vulkan_blender28.py to export a *.vks file
# that includes the geometry for the spherical lights.
lights = [object for object in bpy.context.selected_objects if object.data is not None and object.data.name == "spherical_light"]
with open("scene.lights", "wb") as file:
    file.write(pack("I", len(lights)))
    for light in lights:
        file.write(pack("fff", *light.location))
        file.write(pack("f", light.scale[0]))
