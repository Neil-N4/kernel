#include "fs/ext2.h"
#include "fs/ata.h"
#include "kernel/string.h"
#include "mm/page_cache.h"

static uint8_t block_buffer[EXT2_MAX_BLOCK_SIZE];
static uint8_t block_buffer_2[EXT2_MAX_BLOCK_SIZE];

static uint32_t ceil_div_u32(uint32_t a, uint32_t b)
{
    return (a + b - 1u) / b;
}

static int read_sector_cached(uint32_t lba, void *out)
{
    return page_cache_read_sector(lba, out, ata_read_sectors);
}

static int write_sector_cached(uint32_t lba, const void *in)
{
    return page_cache_write_sector(lba, in, ata_write_sectors);
}

static uint32_t block_to_lba(const ext2_fs_t *fs, uint32_t block)
{
    return fs->partition_lba + block * fs->sectors_per_block;
}

static int read_block(ext2_fs_t *fs, uint32_t block, void *out)
{
    uint8_t *bytes = (uint8_t *)out;
    uint32_t lba = block_to_lba(fs, block);

    for (uint32_t i = 0; i < fs->sectors_per_block; ++i) {
        if (read_sector_cached(lba + i, bytes + i * ATA_SECTOR_SIZE) != 0) {
            return -1;
        }
    }

    return 0;
}

static int write_block(ext2_fs_t *fs, uint32_t block, const void *in)
{
    const uint8_t *bytes = (const uint8_t *)in;
    uint32_t lba = block_to_lba(fs, block);

    for (uint32_t i = 0; i < fs->sectors_per_block; ++i) {
        if (write_sector_cached(lba + i, bytes + i * ATA_SECTOR_SIZE) != 0) {
            return -1;
        }
    }

    return 0;
}

static int read_superblock(ext2_fs_t *fs)
{
    uint8_t sector[ATA_SECTOR_SIZE];
    if (read_sector_cached(fs->partition_lba + 2u, sector) != 0) {
        return -1;
    }
    memcpy(&fs->superblock, sector, sizeof(fs->superblock));
    return fs->superblock.magic == EXT2_SUPER_MAGIC ? 0 : -1;
}

int ext2_mount(ext2_fs_t *fs, uint32_t partition_lba)
{
    memset(fs, 0, sizeof(*fs));
    fs->partition_lba = partition_lba;

    if (read_superblock(fs) != 0) {
        return -1;
    }

    fs->block_size = 1024u << fs->superblock.log_block_size;
    if (fs->block_size > EXT2_MAX_BLOCK_SIZE ||
        fs->block_size < 1024u ||
        (fs->block_size % ATA_SECTOR_SIZE) != 0) {
        return -1;
    }

    fs->sectors_per_block = fs->block_size / ATA_SECTOR_SIZE;
    fs->inode_size = fs->superblock.inode_size == 0 ? 128u
                                                    : fs->superblock.inode_size;
    fs->group_count = ceil_div_u32(fs->superblock.blocks_count,
                                   fs->superblock.blocks_per_group);
    if (fs->group_count > EXT2_MAX_GROUPS) {
        fs->group_count = EXT2_MAX_GROUPS;
    }

    uint32_t gd_table_block = fs->block_size == 1024u ? 2u : 1u;
    if (read_block(fs, gd_table_block, block_buffer) != 0) {
        return -1;
    }

    memcpy(fs->groups, block_buffer, fs->group_count * sizeof(ext2_group_desc_t));
    return 0;
}

int ext2_read_inode(ext2_fs_t *fs, uint32_t inode_no, ext2_inode_t *out)
{
    if (inode_no == 0 || out == 0) {
        return -1;
    }

    uint32_t group = (inode_no - 1u) / fs->superblock.inodes_per_group;
    uint32_t index = (inode_no - 1u) % fs->superblock.inodes_per_group;
    if (group >= fs->group_count) {
        return -1;
    }

    uint32_t byte_offset = index * fs->inode_size;
    uint32_t table_block = fs->groups[group].inode_table;
    uint32_t block = table_block + byte_offset / fs->block_size;
    uint32_t offset = byte_offset % fs->block_size;

    if (read_block(fs, block, block_buffer) != 0) {
        return -1;
    }

    memcpy(out, block_buffer + offset, sizeof(*out));
    return 0;
}

static uint32_t inode_data_block(ext2_fs_t *fs, const ext2_inode_t *inode,
                                 uint32_t logical_block)
{
    if (logical_block < EXT2_NDIR_BLOCKS) {
        return inode->block[logical_block];
    }

    uint32_t indirect_entries = fs->block_size / sizeof(uint32_t);
    logical_block -= EXT2_NDIR_BLOCKS;
    if (logical_block < indirect_entries && inode->block[EXT2_NDIR_BLOCKS] != 0) {
        if (read_block(fs, inode->block[EXT2_NDIR_BLOCKS], block_buffer_2) != 0) {
            return 0;
        }
        const uint32_t *entries = (const uint32_t *)block_buffer_2;
        return entries[logical_block];
    }

    return 0;
}

static int name_equals(const char *component, size_t len,
                       const char *entry_name, uint8_t entry_len)
{
    return len == entry_len && memcmp(component, entry_name, len) == 0;
}

static int lookup_child(ext2_fs_t *fs, uint32_t dir_inode_no,
                        const char *name, size_t name_len, uint32_t *out_inode)
{
    ext2_inode_t dir;
    if (ext2_read_inode(fs, dir_inode_no, &dir) != 0) {
        return -1;
    }

    uint32_t total_blocks = ceil_div_u32(dir.size, fs->block_size);
    uint32_t bytes_left = dir.size;

    for (uint32_t logical = 0; logical < total_blocks; ++logical) {
        uint32_t block = inode_data_block(fs, &dir, logical);
        if (block == 0 || read_block(fs, block, block_buffer) != 0) {
            return -1;
        }

        uint32_t offset = 0;
        uint32_t limit = bytes_left < fs->block_size ? bytes_left : fs->block_size;
        while (offset + sizeof(ext2_dir_entry_t) <= limit) {
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(block_buffer + offset);
            if (entry->rec_len < sizeof(ext2_dir_entry_t) ||
                offset + entry->rec_len > fs->block_size) {
                return -1;
            }

            if (entry->inode != 0 &&
                name_equals(name, name_len, entry->name, entry->name_len)) {
                *out_inode = entry->inode;
                return 0;
            }

            offset += entry->rec_len;
        }

        bytes_left -= limit;
    }

    return -1;
}

int ext2_lookup_path(ext2_fs_t *fs, const char *path, uint32_t *inode_no)
{
    if (fs == 0 || path == 0 || inode_no == 0 || path[0] != '/') {
        return -1;
    }

    uint32_t current = EXT2_ROOT_INO;
    const char *p = path;

    while (*p == '/') {
        ++p;
    }

    while (*p != '\0') {
        const char *start = p;
        while (*p != '\0' && *p != '/') {
            ++p;
        }

        size_t len = (size_t)(p - start);
        if (len == 0 || len > EXT2_NAME_MAX) {
            return -1;
        }

        if (lookup_child(fs, current, start, len, &current) != 0) {
            return -1;
        }

        while (*p == '/') {
            ++p;
        }
    }

    *inode_no = current;
    return 0;
}

int ext2_read_file(ext2_fs_t *fs, uint32_t inode_no, uint32_t offset,
                   void *out, size_t count)
{
    ext2_inode_t inode;
    if (ext2_read_inode(fs, inode_no, &inode) != 0 || out == 0) {
        return -1;
    }

    if (offset >= inode.size) {
        return 0;
    }

    if (offset + count > inode.size) {
        count = inode.size - offset;
    }

    uint8_t *dst = (uint8_t *)out;
    size_t copied = 0;

    while (copied < count) {
        uint32_t absolute = offset + (uint32_t)copied;
        uint32_t logical = absolute / fs->block_size;
        uint32_t block_offset = absolute % fs->block_size;
        uint32_t block = inode_data_block(fs, &inode, logical);
        if (block == 0 || read_block(fs, block, block_buffer) != 0) {
            return -1;
        }

        size_t available = fs->block_size - block_offset;
        size_t needed = count - copied;
        size_t chunk = available < needed ? available : needed;
        memcpy(dst + copied, block_buffer + block_offset, chunk);
        copied += chunk;
    }

    return (int)copied;
}

static int bitmap_test(const uint8_t *bitmap, uint32_t bit)
{
    return (bitmap[bit / 8u] & (1u << (bit % 8u))) != 0;
}

static void bitmap_set(uint8_t *bitmap, uint32_t bit)
{
    bitmap[bit / 8u] |= (uint8_t)(1u << (bit % 8u));
}

int ext2_alloc_block_near(ext2_fs_t *fs, uint32_t goal_block, uint32_t *out_block)
{
    if (fs == 0 || out_block == 0) {
        return -1;
    }

    uint32_t group = goal_block / fs->superblock.blocks_per_group;
    if (group >= fs->group_count) {
        group = 0;
    }

    for (uint32_t group_offset = 0; group_offset < fs->group_count; ++group_offset) {
        uint32_t g = (group + group_offset) % fs->group_count;
        uint32_t bitmap_block = fs->groups[g].block_bitmap;

        if (read_block(fs, bitmap_block, block_buffer) != 0) {
            return -1;
        }

        uint32_t group_start = g * fs->superblock.blocks_per_group;
        uint32_t start = 0;
        if (goal_block >= group_start &&
            goal_block < group_start + fs->superblock.blocks_per_group) {
            start = goal_block - group_start;
        }

        for (uint32_t pass = 0; pass < 2; ++pass) {
            uint32_t begin = pass == 0 ? start : 0;
            uint32_t end = pass == 0 ? fs->superblock.blocks_per_group : start;

            for (uint32_t bit = begin; bit < end; ++bit) {
                uint32_t absolute = group_start + bit;
                if (absolute >= fs->superblock.blocks_count) {
                    break;
                }

                if (!bitmap_test(block_buffer, bit)) {
                    bitmap_set(block_buffer, bit);
                    if (write_block(fs, bitmap_block, block_buffer) != 0) {
                        return -1;
                    }

                    if (fs->groups[g].free_blocks_count > 0) {
                        --fs->groups[g].free_blocks_count;
                    }
                    if (fs->superblock.free_blocks_count > 0) {
                        --fs->superblock.free_blocks_count;
                    }

                    *out_block = absolute;
                    return 0;
                }
            }
        }
    }

    return -1;
}
