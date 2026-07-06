#include "mm/page_cache.h"
#include "kernel/string.h"

typedef struct page_cache_line {
    uint8_t valid;
    uint8_t dirty;
    uint16_t reserved;
    uint32_t lba;
    uint8_t data[PAGE_CACHE_LINE_SIZE];
} page_cache_line_t;

static page_cache_line_t cache[PAGE_CACHE_LINES];

static uint32_t cache_index(uint32_t lba)
{
    return lba % PAGE_CACHE_LINES;
}

void page_cache_init(void)
{
    memset(cache, 0, sizeof(cache));
}

int page_cache_read_sector(uint32_t lba, void *out, page_cache_reader_t reader)
{
    page_cache_line_t *line = &cache[cache_index(lba)];
    if (!line->valid || line->lba != lba) {
        if (reader == 0 || reader(lba, 1, line->data) != 0) {
            return -1;
        }
        line->valid = 1;
        line->dirty = 0;
        line->lba = lba;
    }

    memcpy(out, line->data, PAGE_CACHE_LINE_SIZE);
    return 0;
}

int page_cache_write_sector(uint32_t lba, const void *in, page_cache_writer_t writer)
{
    page_cache_line_t *line = &cache[cache_index(lba)];
    memcpy(line->data, in, PAGE_CACHE_LINE_SIZE);
    line->valid = 1;
    line->dirty = 1;
    line->lba = lba;

    if (writer != 0) {
        if (writer(lba, 1, line->data) != 0) {
            return -1;
        }
        line->dirty = 0;
    }

    return 0;
}

void page_cache_invalidate(uint32_t lba)
{
    page_cache_line_t *line = &cache[cache_index(lba)];
    if (line->valid && line->lba == lba) {
        line->valid = 0;
        line->dirty = 0;
    }
}

void page_cache_invalidate_all(void)
{
    memset(cache, 0, sizeof(cache));
}
