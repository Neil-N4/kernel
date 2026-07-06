#ifndef FS_EXT2_H
#define FS_EXT2_H

#include <stddef.h>
#include <stdint.h>

#define EXT2_SUPER_MAGIC       0xEF53u
#define EXT2_ROOT_INO          2u
#define EXT2_NAME_MAX          255u
#define EXT2_MAX_BLOCK_SIZE    4096u
#define EXT2_MAX_GROUPS        32u
#define EXT2_N_BLOCKS          15u
#define EXT2_NDIR_BLOCKS       12u

typedef struct ext2_superblock {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t reserved_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t algo_bitmap;
} __attribute__((packed)) ext2_superblock_t;

typedef struct ext2_group_desc {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint8_t reserved[12];
} __attribute__((packed)) ext2_group_desc_t;

typedef struct ext2_inode {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[EXT2_N_BLOCKS];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint8_t osd2[12];
} __attribute__((packed)) ext2_inode_t;

typedef struct ext2_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
} __attribute__((packed)) ext2_dir_entry_t;

typedef struct ext2_fs {
    uint32_t partition_lba;
    uint32_t block_size;
    uint32_t sectors_per_block;
    uint32_t group_count;
    uint16_t inode_size;
    ext2_superblock_t superblock;
    ext2_group_desc_t groups[EXT2_MAX_GROUPS];
} ext2_fs_t;

int ext2_mount(ext2_fs_t *fs, uint32_t partition_lba);
int ext2_read_inode(ext2_fs_t *fs, uint32_t inode_no, ext2_inode_t *out);
int ext2_lookup_path(ext2_fs_t *fs, const char *path, uint32_t *inode_no);
int ext2_read_file(ext2_fs_t *fs, uint32_t inode_no, uint32_t offset,
                   void *out, size_t count);
int ext2_alloc_block_near(ext2_fs_t *fs, uint32_t goal_block, uint32_t *out_block);

#endif
