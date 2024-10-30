import re
from xml.dom.minidom import parse


def load_formats(vk_xml_path="vk.xml"):
    """
    Extracts all <format> tags from the Vulkan XML specification.
    :param vk_xml_path: Path to the vk.xml file.
    :return: Tuples of strings (name, cls, block_size, texels_per_block,
        packed_bits) some of which may be empty.
    """
    doc = parse(vk_xml_path)
    attrs = ["name", "class", "blockSize", "texelsPerBlock", "packed"]
    return [[format.getAttribute(attr) for attr in attrs] for format in doc.getElementsByTagName("format")]


def format_class(cls):
    """Turns the class attribute in the XML into the name of an enum entry."""
    return "FORMAT_CLASS_" + re.sub(r"\W", r"_", cls.upper())


def generate_vulkan_formats():
    """Generates vulkan_formats.h and vulkan_formats.c in ../src."""
    classes = list()
    with open("../src/vulkan_formats.c", "w") as file:
        file.write(
"""#include \"vulkan_formats.h\"


format_description_t get_format_description(VkFormat format) {
\tformat_description_t desc = { .packed_bits = 0 };
\tswitch (format) {
"""
        )
        for name, cls, block_size, texels_per_block, packed_bits in load_formats():
            if cls not in classes:
                classes.append(cls)
            file.write("\t\tcase %s:\n" % name)
            file.write("\t\t\tdesc.cls = %s;\n" % format_class(cls))
            file.write("\t\t\tdesc.block_size = %s;\n" % block_size)
            file.write("\t\t\tdesc.texels_per_block = %s;\n" % texels_per_block)
            if len(packed_bits):
                file.write("\t\t\tdesc.packed_bits = %s;\n" % packed_bits)
            file.write("\t\t\tbreak;\n")
        file.write(
"""\t\tdefault:
\t\t\tbreak;
\t}
\treturn desc;
}\n"""
        )
    with open("../src/vulkan_formats.h", "w") as file:
        file.write(
"""#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdint.h>


//! Enumeration of all possible format compatibility classes
typedef enum {
"""
        )
        for cls in classes:
            file.write("\t%s,\n" % format_class(cls))
        file.write(
"""} format_class_t;


//! Provides meta data about a Vulkan format that can for example be used to
//! figure out how to copy data from a buffer to an image. Autogenerated from
//! vk.xml by ../tools/vulkan_formats.py.
typedef struct {
\t//! The format compatibility class of the format. Copying between two
\t//! formats of the same class is permitted.
\tformat_class_t cls;
\t//! The number of bytes per texel block
\tVkDeviceSize block_size;
\t//! The number of texels per texel block
\tuint32_t texels_per_block;
\t//! For packed formats such as VK_FORMAT_R5G6B5_UNORM_PACK16, this is the
\t//! number of bits into which a color is being packed. 0 otherwise.
\tuint32_t packed_bits;
} format_description_t;


//! Provides meta data about the given Vulkan format.
//! \see format_description_t
format_description_t get_format_description(VkFormat format);\n"""
        )


if __name__ == "__main__":
    generate_vulkan_formats()