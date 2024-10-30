#include "vulkan_formats.h"


format_description_t get_format_description(VkFormat format) {
	format_description_t desc = { .packed_bits = 0 };
	switch (format) {
		case VK_FORMAT_R4G4_UNORM_PACK8:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			desc.packed_bits = 8;
			break;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_A8_UNORM_KHR:
			desc.cls = FORMAT_CLASS_8_BIT_ALPHA;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8_UNORM:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8_SNORM:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8_USCALED:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8_SSCALED:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8_UINT:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8_SINT:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8_SRGB:
			desc.cls = FORMAT_CLASS_8_BIT;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8_SNORM:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8_USCALED:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8_SSCALED:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8_UINT:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8_SINT:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8_SRGB:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8_UNORM:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8_SNORM:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8_USCALED:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8_SSCALED:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8_UINT:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8_SINT:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8_SRGB:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8_UNORM:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8_SNORM:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8_USCALED:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8_SSCALED:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8_UINT:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8_SINT:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8_SRGB:
			desc.cls = FORMAT_CLASS_24_BIT;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8A8_SNORM:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8A8_USCALED:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8A8_SSCALED:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8A8_UINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8A8_SINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R8G8B8A8_SRGB:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8A8_UNORM:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8A8_SNORM:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8A8_USCALED:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8A8_SSCALED:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8A8_UINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8A8_SINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8A8_SRGB:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_R16_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16_SNORM:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16_USCALED:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16_SSCALED:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16_UINT:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16_SINT:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16_SFLOAT:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16_UNORM:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16_SNORM:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16_USCALED:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16_SSCALED:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16_UINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16_SINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16_SFLOAT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16_UNORM:
			desc.cls = FORMAT_CLASS_48_BIT;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16_SNORM:
			desc.cls = FORMAT_CLASS_48_BIT;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16_USCALED:
			desc.cls = FORMAT_CLASS_48_BIT;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16_SSCALED:
			desc.cls = FORMAT_CLASS_48_BIT;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16_UINT:
			desc.cls = FORMAT_CLASS_48_BIT;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16_SINT:
			desc.cls = FORMAT_CLASS_48_BIT;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16_SFLOAT:
			desc.cls = FORMAT_CLASS_48_BIT;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16A16_UNORM:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16A16_SNORM:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16A16_USCALED:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16A16_SSCALED:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16A16_UINT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16A16_SINT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32_UINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32_SINT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32_SFLOAT:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32_UINT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32_SINT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32_SFLOAT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32B32_UINT:
			desc.cls = FORMAT_CLASS_96_BIT;
			desc.block_size = 12;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32B32_SINT:
			desc.cls = FORMAT_CLASS_96_BIT;
			desc.block_size = 12;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32B32_SFLOAT:
			desc.cls = FORMAT_CLASS_96_BIT;
			desc.block_size = 12;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32B32A32_UINT:
			desc.cls = FORMAT_CLASS_128_BIT;
			desc.block_size = 16;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32B32A32_SINT:
			desc.cls = FORMAT_CLASS_128_BIT;
			desc.block_size = 16;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			desc.cls = FORMAT_CLASS_128_BIT;
			desc.block_size = 16;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64_UINT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64_SINT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64_SFLOAT:
			desc.cls = FORMAT_CLASS_64_BIT;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64_UINT:
			desc.cls = FORMAT_CLASS_128_BIT;
			desc.block_size = 16;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64_SINT:
			desc.cls = FORMAT_CLASS_128_BIT;
			desc.block_size = 16;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64_SFLOAT:
			desc.cls = FORMAT_CLASS_128_BIT;
			desc.block_size = 16;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64B64_UINT:
			desc.cls = FORMAT_CLASS_192_BIT;
			desc.block_size = 24;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64B64_SINT:
			desc.cls = FORMAT_CLASS_192_BIT;
			desc.block_size = 24;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64B64_SFLOAT:
			desc.cls = FORMAT_CLASS_192_BIT;
			desc.block_size = 24;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64B64A64_UINT:
			desc.cls = FORMAT_CLASS_256_BIT;
			desc.block_size = 32;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64B64A64_SINT:
			desc.cls = FORMAT_CLASS_256_BIT;
			desc.block_size = 32;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			desc.cls = FORMAT_CLASS_256_BIT;
			desc.block_size = 32;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_D16_UNORM:
			desc.cls = FORMAT_CLASS_D16;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			desc.cls = FORMAT_CLASS_D24;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 32;
			break;
		case VK_FORMAT_D32_SFLOAT:
			desc.cls = FORMAT_CLASS_D32;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_S8_UINT:
			desc.cls = FORMAT_CLASS_S8;
			desc.block_size = 1;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_D16_UNORM_S8_UINT:
			desc.cls = FORMAT_CLASS_D16S8;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			desc.cls = FORMAT_CLASS_D24S8;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			desc.cls = FORMAT_CLASS_D32S8;
			desc.block_size = 5;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC1_RGB;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_BC1_RGB;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC1_RGBA;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_BC1_RGBA;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC2_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC2;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC2_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_BC2;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC3_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC3;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC3_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_BC3;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC4_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC4;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC4_SNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC4;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC5_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC5;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC5_SNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC5;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_BC6H;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_BC6H;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC7_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_BC7;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_BC7_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_BC7;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ETC2_RGB;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ETC2_RGB;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ETC2_RGBA;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ETC2_RGBA;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ETC2_EAC_RGBA;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ETC2_EAC_RGBA;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_EAC_R11_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_EAC_R;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_EAC_R11_SNORM_BLOCK:
			desc.cls = FORMAT_CLASS_EAC_R;
			desc.block_size = 8;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_EAC_RG;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
			desc.cls = FORMAT_CLASS_EAC_RG;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_4X4;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_4X4;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_5X4;
			desc.block_size = 16;
			desc.texels_per_block = 20;
			break;
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_5X4;
			desc.block_size = 16;
			desc.texels_per_block = 20;
			break;
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_5X5;
			desc.block_size = 16;
			desc.texels_per_block = 25;
			break;
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_5X5;
			desc.block_size = 16;
			desc.texels_per_block = 25;
			break;
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_6X5;
			desc.block_size = 16;
			desc.texels_per_block = 30;
			break;
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_6X5;
			desc.block_size = 16;
			desc.texels_per_block = 30;
			break;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_6X6;
			desc.block_size = 16;
			desc.texels_per_block = 36;
			break;
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_6X6;
			desc.block_size = 16;
			desc.texels_per_block = 36;
			break;
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X5;
			desc.block_size = 16;
			desc.texels_per_block = 40;
			break;
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X5;
			desc.block_size = 16;
			desc.texels_per_block = 40;
			break;
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X6;
			desc.block_size = 16;
			desc.texels_per_block = 48;
			break;
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X6;
			desc.block_size = 16;
			desc.texels_per_block = 48;
			break;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X8;
			desc.block_size = 16;
			desc.texels_per_block = 64;
			break;
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X8;
			desc.block_size = 16;
			desc.texels_per_block = 64;
			break;
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X5;
			desc.block_size = 16;
			desc.texels_per_block = 50;
			break;
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X5;
			desc.block_size = 16;
			desc.texels_per_block = 50;
			break;
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X6;
			desc.block_size = 16;
			desc.texels_per_block = 60;
			break;
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X6;
			desc.block_size = 16;
			desc.texels_per_block = 60;
			break;
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X8;
			desc.block_size = 16;
			desc.texels_per_block = 80;
			break;
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X8;
			desc.block_size = 16;
			desc.texels_per_block = 80;
			break;
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X10;
			desc.block_size = 16;
			desc.texels_per_block = 100;
			break;
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X10;
			desc.block_size = 16;
			desc.texels_per_block = 100;
			break;
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_12X10;
			desc.block_size = 16;
			desc.texels_per_block = 120;
			break;
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_12X10;
			desc.block_size = 16;
			desc.texels_per_block = 120;
			break;
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_12X12;
			desc.block_size = 16;
			desc.texels_per_block = 144;
			break;
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_12X12;
			desc.block_size = 16;
			desc.texels_per_block = 144;
			break;
		case VK_FORMAT_G8B8G8R8_422_UNORM:
			desc.cls = FORMAT_CLASS_32_BIT_G8B8G8R8;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B8G8R8G8_422_UNORM:
			desc.cls = FORMAT_CLASS_32_BIT_B8G8R8G8;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
			desc.cls = FORMAT_CLASS_8_BIT_3_PLANE_420;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
			desc.cls = FORMAT_CLASS_8_BIT_2_PLANE_420;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
			desc.cls = FORMAT_CLASS_8_BIT_3_PLANE_422;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
			desc.cls = FORMAT_CLASS_8_BIT_2_PLANE_422;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
			desc.cls = FORMAT_CLASS_8_BIT_3_PLANE_444;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_R10X6_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
			desc.cls = FORMAT_CLASS_64_BIT_R10G10B10A10;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
			desc.cls = FORMAT_CLASS_64_BIT_G10B10G10R10;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
			desc.cls = FORMAT_CLASS_64_BIT_B10G10R10G10;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_10_BIT_3_PLANE_420;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_10_BIT_2_PLANE_420;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_10_BIT_3_PLANE_422;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_10_BIT_2_PLANE_422;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_10_BIT_3_PLANE_444;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R12X4_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
			desc.cls = FORMAT_CLASS_64_BIT_R12G12B12A12;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
			desc.cls = FORMAT_CLASS_64_BIT_G12B12G12R12;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
			desc.cls = FORMAT_CLASS_64_BIT_B12G12R12G12;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_12_BIT_3_PLANE_420;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_12_BIT_2_PLANE_420;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_12_BIT_3_PLANE_422;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_12_BIT_2_PLANE_422;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_12_BIT_3_PLANE_444;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G16B16G16R16_422_UNORM:
			desc.cls = FORMAT_CLASS_64_BIT_G16B16G16R16;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_B16G16R16G16_422_UNORM:
			desc.cls = FORMAT_CLASS_64_BIT_B16G16R16G16;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT_3_PLANE_420;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT_2_PLANE_420;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT_3_PLANE_422;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT_2_PLANE_422;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT_3_PLANE_444;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC1_2BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC1_4BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC2_2BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC2_4BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC1_2BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC1_4BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC2_2BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
			desc.cls = FORMAT_CLASS_PVRTC2_4BPP;
			desc.block_size = 8;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_4X4;
			desc.block_size = 16;
			desc.texels_per_block = 16;
			break;
		case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_5X4;
			desc.block_size = 16;
			desc.texels_per_block = 20;
			break;
		case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_5X5;
			desc.block_size = 16;
			desc.texels_per_block = 25;
			break;
		case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_6X5;
			desc.block_size = 16;
			desc.texels_per_block = 30;
			break;
		case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_6X6;
			desc.block_size = 16;
			desc.texels_per_block = 36;
			break;
		case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X5;
			desc.block_size = 16;
			desc.texels_per_block = 40;
			break;
		case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X6;
			desc.block_size = 16;
			desc.texels_per_block = 48;
			break;
		case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_8X8;
			desc.block_size = 16;
			desc.texels_per_block = 64;
			break;
		case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X5;
			desc.block_size = 16;
			desc.texels_per_block = 50;
			break;
		case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X6;
			desc.block_size = 16;
			desc.texels_per_block = 60;
			break;
		case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X8;
			desc.block_size = 16;
			desc.texels_per_block = 80;
			break;
		case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_10X10;
			desc.block_size = 16;
			desc.texels_per_block = 100;
			break;
		case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_12X10;
			desc.block_size = 16;
			desc.texels_per_block = 120;
			break;
		case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
			desc.cls = FORMAT_CLASS_ASTC_12X12;
			desc.block_size = 16;
			desc.texels_per_block = 144;
			break;
		case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
			desc.cls = FORMAT_CLASS_8_BIT_2_PLANE_444;
			desc.block_size = 3;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_10_BIT_2_PLANE_444;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
			desc.cls = FORMAT_CLASS_12_BIT_2_PLANE_444;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
			desc.cls = FORMAT_CLASS_16_BIT_2_PLANE_444;
			desc.block_size = 6;
			desc.texels_per_block = 1;
			break;
		case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
			desc.cls = FORMAT_CLASS_16_BIT;
			desc.block_size = 2;
			desc.texels_per_block = 1;
			desc.packed_bits = 16;
			break;
		case VK_FORMAT_R16G16_S10_5_NV:
			desc.cls = FORMAT_CLASS_32_BIT;
			desc.block_size = 4;
			desc.texels_per_block = 1;
			break;
		default:
			break;
	}
	return desc;
}
