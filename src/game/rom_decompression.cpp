/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * ROM Decompression
 *
 * Handles MIO0 decompression for compressed ROM data.
 * Midway N64 titles commonly use MIO0 compression.
 * This implementation is based on the Rampage Recompiled decompressor.
 *
 * MIO0 Format:
 *   Header (16 bytes):
 *     0x00: "MIO0" magic
 *     0x04: Uncompressed size (big-endian 32-bit)
 *     0x08: Offset to compressed data
 *     0x0C: Offset to uncompressed data
 *   Data:
 *     Layout bits (1 = uncompressed byte, 0 = LZ77 back-reference)
 *     Compressed stream (16-bit back-references: length + offset)
 *     Uncompressed stream (literal bytes)
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>

static uint32_t read_be32(const uint8_t* data) {
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] <<  8) |
           ((uint32_t)data[3]);
}

static uint16_t read_be16(const uint8_t* data) {
    return ((uint16_t)data[0] << 8) | data[1];
}

namespace sfrush {

/**
 * Check if data at the given pointer is MIO0 compressed.
 */
bool mio0_is_compressed(const uint8_t* data) {
    return data[0] == 'M' && data[1] == 'I' && data[2] == 'O' && data[3] == '0';
}

/**
 * Decompress MIO0 data.
 * Returns allocated buffer (caller must free) and sets out_size.
 * Returns nullptr on failure.
 */
uint8_t* mio0_decompress(const uint8_t* src, uint32_t* out_size) {
    if (!mio0_is_compressed(src)) {
        fprintf(stderr, "[MIO0] Invalid magic\n");
        return nullptr;
    }

    uint32_t uncompressed_size = read_be32(src + 0x04);
    uint32_t comp_offset = read_be32(src + 0x08);
    uint32_t uncomp_offset = read_be32(src + 0x0C);

    uint8_t* dst = (uint8_t*)malloc(uncompressed_size);
    if (!dst) {
        fprintf(stderr, "[MIO0] Failed to allocate %u bytes\n", uncompressed_size);
        return nullptr;
    }

    uint32_t dst_pos = 0;
    uint32_t layout_pos = 0x10; // Layout bits start after header
    uint32_t layout_bit = 0;
    uint8_t layout_byte = src[layout_pos];

    const uint8_t* comp_ptr = src + comp_offset;
    const uint8_t* uncomp_ptr = src + uncomp_offset;

    while (dst_pos < uncompressed_size) {
        if (layout_byte & (0x80 >> layout_bit)) {
            // Uncompressed byte
            dst[dst_pos++] = *uncomp_ptr++;
        } else {
            // LZ77 back-reference
            uint16_t ref = read_be16(comp_ptr);
            comp_ptr += 2;

            uint32_t length = ((ref >> 12) & 0x0F) + 3;
            uint32_t offset = (ref & 0x0FFF) + 1;

            for (uint32_t i = 0; i < length && dst_pos < uncompressed_size; i++) {
                dst[dst_pos] = dst[dst_pos - offset];
                dst_pos++;
            }
        }

        layout_bit++;
        if (layout_bit >= 8) {
            layout_bit = 0;
            layout_pos++;
            layout_byte = src[layout_pos];
        }
    }

    if (out_size) {
        *out_size = uncompressed_size;
    }

    fprintf(stderr, "[MIO0] Decompressed %u bytes\n", uncompressed_size);
    return dst;
}

} // namespace sfrush
