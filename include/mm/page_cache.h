#ifndef MM_PAGE_CACHE_H
#define MM_PAGE_CACHE_H

#include <stdint.h>

#define PAGE_CACHE_LINE_SIZE 512u
#define PAGE_CACHE_LINES     128u

typedef int (*page_cache_reader_t)(uint32_t lba, uint8_t sectors, void *buffer);
typedef int (*page_cache_writer_t)(uint32_t lba, uint8_t sectors, const void *buffer);

void page_cache_init(void);
int page_cache_read_sector(uint32_t lba, void *out, page_cache_reader_t reader);
int page_cache_write_sector(uint32_t lba, const void *in, page_cache_writer_t writer);
void page_cache_invalidate(uint32_t lba);
void page_cache_invalidate_all(void);

#endif
