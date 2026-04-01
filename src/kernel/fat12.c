#include "fat12.h"

#include "ata.h"
#include "memory.h"

#define FAT12_SECTOR_SIZE 512u

// FAT12 cluster end-of-chain marker range begins at 0xFF8.
#define FAT12_EOC_MIN 0x0FF8u
#define FAT12_EOC     0x0FFFu

#define FAT_ATTR_READONLY 0x01u
#define FAT_ATTR_HIDDEN   0x02u
#define FAT_ATTR_SYSTEM   0x04u
#define FAT_ATTR_VOLUME   0x08u
#define FAT_ATTR_DIR      0x10u
#define FAT_ATTR_ARCHIVE  0x20u
#define FAT_ATTR_LFN_MASK 0x0Fu

typedef struct __attribute__((packed))
{
    uint8_t name[11];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} fat_dirent_t;

typedef struct
{
    uint8_t ready;

    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entry_count;
    uint32_t total_sectors;
    uint16_t sectors_per_fat;

    uint32_t root_dir_sectors;
    uint32_t fat_lba;
    uint32_t root_dir_lba;
    uint32_t data_lba;

    uint32_t cluster_count;

    uint8_t* fat;
    uint32_t fat_bytes;

    uint8_t* root_dir;
    uint32_t root_dir_bytes;
} fat12_state_t;

static fat12_state_t g_Fs;

static void mem_zero(void* p, uint32_t n)
{
    uint8_t* b = (uint8_t*)p;
    for (uint32_t i = 0; i < n; i++)
        b[i] = 0;
}

static void mem_copy(void* dst, const void* src, uint32_t n)
{
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++)
        d[i] = s[i];
}

static uint16_t read_u16(const uint8_t* p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32(const uint8_t* p)
{
    return (uint32_t)read_u16(p) | ((uint32_t)read_u16(p + 2) << 16);
}

static uint8_t to_upper(uint8_t c)
{
    if (c >= 'a' && c <= 'z')
        return (uint8_t)(c - ('a' - 'A'));
    return c;
}

static uint8_t to_lower(uint8_t c)
{
    if (c >= 'A' && c <= 'Z')
        return (uint8_t)(c + ('a' - 'A'));
    return c;
}

static int is_name_char(uint8_t c)
{
    // Strict 8.3 subset (good enough for notepad): A-Z, 0-9, '_' and '-'.
    if (c >= 'A' && c <= 'Z')
        return 1;
    if (c >= '0' && c <= '9')
        return 1;
    if (c == '_' || c == '-')
        return 1;

    return 0;
}

static int fat12_format_name11(const char* name, uint8_t out[11])
{
    if (!name || !name[0] || !out)
        return 0;

    for (uint32_t i = 0; i < 11; i++)
        out[i] = ' ';

    // Reject paths.
    for (const char* p = name; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            return 0;
    }

    const char* dot = 0;
    for (const char* p = name; *p; p++)
    {
        if (*p == '.')
        {
            if (dot)
                return 0;
            dot = p;
        }
    }

    uint32_t base_len = 0;
    while (name[base_len] && name[base_len] != '.')
        base_len++;

    uint32_t ext_len = 0;
    if (dot && dot[1])
    {
        const char* ext = dot + 1;
        while (ext[ext_len])
            ext_len++;
    }

    if (base_len == 0 || base_len > 8)
        return 0;
    if (ext_len > 3)
        return 0;

    for (uint32_t i = 0; i < base_len; i++)
    {
        uint8_t c = to_upper((uint8_t)name[i]);
        if (!is_name_char(c))
            return 0;
        out[i] = c;
    }

    if (ext_len)
    {
        const char* ext = dot + 1;
        for (uint32_t i = 0; i < ext_len; i++)
        {
            uint8_t c = to_upper((uint8_t)ext[i]);
            if (!is_name_char(c))
                return 0;
            out[8u + i] = c;
        }
    }

    return 1;
}

int fat12_is_valid_name(const char* name)
{
    uint8_t name11[11];
    return fat12_format_name11(name, name11);
}

static int name11_eq(const uint8_t a[11], const uint8_t b[11])
{
    for (uint32_t i = 0; i < 11; i++)
    {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

static void name11_to_str(const uint8_t name11[11], char out[32])
{
    // Convert 8.3 into "name.ext" (lowercase), trimming trailing spaces.
    uint32_t n = 0;

    // Base.
    int last_non_space = -1;
    for (int i = 0; i < 8; i++)
    {
        if (name11[i] != ' ')
            last_non_space = i;
    }

    for (int i = 0; i <= last_non_space && n + 1 < 32; i++)
        out[n++] = (char)to_lower(name11[i]);

    // Ext.
    last_non_space = -1;
    for (int i = 0; i < 3; i++)
    {
        if (name11[8 + i] != ' ')
            last_non_space = i;
    }

    if (last_non_space >= 0 && n + 2 < 32)
    {
        out[n++] = '.';
        for (int i = 0; i <= last_non_space && n + 1 < 32; i++)
            out[n++] = (char)to_lower(name11[8 + i]);
    }

    out[n] = 0;
}

static fat_dirent_t* root_entry(uint32_t i)
{
    return (fat_dirent_t*)(g_Fs.root_dir + i * 32u);
}

static int dirent_is_valid_file(const fat_dirent_t* e)
{
    if (!e)
        return 0;

    uint8_t first = e->name[0];
    if (first == 0x00u)
        return 0;
    if (first == 0xE5u)
        return 0;

    uint8_t attr = e->attr;
    if ((attr & FAT_ATTR_LFN_MASK) == FAT_ATTR_LFN_MASK)
        return 0;
    if (attr & FAT_ATTR_VOLUME)
        return 0;
    if (attr & FAT_ATTR_DIR)
        return 0;

    return 1;
}

static int fat12_find_root(const uint8_t name11[11], uint32_t* out_index)
{
    if (!g_Fs.ready)
        return 0;

    for (uint32_t i = 0; i < g_Fs.root_entry_count; i++)
    {
        fat_dirent_t* e = root_entry(i);
        uint8_t first = e->name[0];
        if (first == 0x00u)
            break;
        if (first == 0xE5u)
            continue;
        if (!dirent_is_valid_file(e))
            continue;

        if (name11_eq(e->name, name11))
        {
            if (out_index)
                *out_index = i;
            return 1;
        }
    }

    return 0;
}

static int fat12_find_free_root(uint32_t* out_index)
{
    if (!g_Fs.ready)
        return 0;

    for (uint32_t i = 0; i < g_Fs.root_entry_count; i++)
    {
        fat_dirent_t* e = root_entry(i);
        uint8_t first = e->name[0];
        if (first == 0x00u || first == 0xE5u)
        {
            if (out_index)
                *out_index = i;
            return 1;
        }
    }

    return 0;
}

static uint16_t fat12_get(uint16_t cluster)
{
    if (!g_Fs.fat)
        return 0;

    uint32_t offset = (uint32_t)cluster + ((uint32_t)cluster / 2u);
    if (offset + 1 >= g_Fs.fat_bytes)
        return FAT12_EOC;

    uint16_t entry = (uint16_t)g_Fs.fat[offset] | ((uint16_t)g_Fs.fat[offset + 1] << 8);
    if (cluster & 1u)
        return (uint16_t)(entry >> 4);

    return (uint16_t)(entry & 0x0FFFu);
}

static void fat12_set(uint16_t cluster, uint16_t value)
{
    if (!g_Fs.fat)
        return;

    uint32_t offset = (uint32_t)cluster + ((uint32_t)cluster / 2u);
    if (offset + 1 >= g_Fs.fat_bytes)
        return;

    uint16_t v = (uint16_t)(value & 0x0FFFu);

    if (cluster & 1u)
    {
        // odd cluster: high 12 bits
        g_Fs.fat[offset] = (uint8_t)((g_Fs.fat[offset] & 0x0Fu) | ((v << 4) & 0xF0u));
        g_Fs.fat[offset + 1] = (uint8_t)((v >> 4) & 0xFFu);
    }
    else
    {
        // even cluster: low 12 bits
        g_Fs.fat[offset] = (uint8_t)(v & 0xFFu);
        g_Fs.fat[offset + 1] = (uint8_t)((g_Fs.fat[offset + 1] & 0xF0u) | ((v >> 8) & 0x0Fu));
    }
}

static uint16_t fat12_alloc_cluster(void)
{
    // Clusters 0 and 1 are reserved.
    for (uint32_t n = 2; n < (g_Fs.cluster_count + 2u); n++)
    {
        if (fat12_get((uint16_t)n) == 0)
        {
            fat12_set((uint16_t)n, FAT12_EOC);
            return (uint16_t)n;
        }
    }

    return 0;
}

static void fat12_free_chain(uint16_t first)
{
    if (!first)
        return;

    uint16_t c = first;
    for (uint32_t i = 0; i < g_Fs.cluster_count + 2u; i++)
    {
        if (c < 2)
            break;
        if (c >= FAT12_EOC_MIN)
            break;

        uint16_t next = fat12_get(c);
        fat12_set(c, 0);
        c = next;
    }
}

static uint32_t cluster_lba(uint16_t cluster)
{
    return g_Fs.data_lba + (uint32_t)(cluster - 2u) * (uint32_t)g_Fs.sectors_per_cluster;
}

static int fat12_sync(void)
{
    if (!g_Fs.ready)
        return 0;

    if (g_Fs.sectors_per_fat == 0 || g_Fs.root_dir_sectors == 0)
        return 0;

    if (g_Fs.sectors_per_fat > 255u || g_Fs.root_dir_sectors > 255u)
        return 0;

    // Write all FAT copies.
    for (uint32_t i = 0; i < g_Fs.fat_count; i++)
    {
        uint32_t lba = g_Fs.fat_lba + i * (uint32_t)g_Fs.sectors_per_fat;
        if (!ata_write_sectors(lba, (uint8_t)g_Fs.sectors_per_fat, g_Fs.fat))
            return 0;
    }

    // Write root directory.
    if (!ata_write_sectors(g_Fs.root_dir_lba, (uint8_t)g_Fs.root_dir_sectors, g_Fs.root_dir))
        return 0;

    return 1;
}

int fat12_ready(void)
{
    return g_Fs.ready ? 1 : 0;
}

int fat12_init(void)
{
    // Re-init: free previous caches.
    if (g_Fs.fat)
        kfree(g_Fs.fat);
    if (g_Fs.root_dir)
        kfree(g_Fs.root_dir);

    mem_zero(&g_Fs, (uint32_t)sizeof(g_Fs));

    if (!ata_init())
        return 0;

    uint8_t bpb[FAT12_SECTOR_SIZE];
    if (!ata_read_sectors(0, 1, bpb))
        return 0;

    if (bpb[510] != 0x55u || bpb[511] != 0xAAu)
        return 0;

    g_Fs.bytes_per_sector = read_u16(&bpb[11]);
    g_Fs.sectors_per_cluster = bpb[13];
    g_Fs.reserved_sectors = read_u16(&bpb[14]);
    g_Fs.fat_count = bpb[16];
    g_Fs.root_entry_count = read_u16(&bpb[17]);

    uint16_t total16 = read_u16(&bpb[19]);
    uint32_t total32 = read_u32(&bpb[32]);
    g_Fs.total_sectors = total16 ? (uint32_t)total16 : total32;

    g_Fs.sectors_per_fat = read_u16(&bpb[22]);

    if (g_Fs.bytes_per_sector != FAT12_SECTOR_SIZE)
        return 0;
    if (g_Fs.sectors_per_cluster == 0)
        return 0;
    if (g_Fs.reserved_sectors == 0)
        return 0;
    if (g_Fs.fat_count == 0 || g_Fs.fat_count > 2)
        return 0;
    if (g_Fs.root_entry_count == 0)
        return 0;
    if (g_Fs.sectors_per_fat == 0)
        return 0;
    if (g_Fs.total_sectors == 0)
        return 0;

    g_Fs.root_dir_sectors = (uint32_t)((g_Fs.root_entry_count * 32u + (FAT12_SECTOR_SIZE - 1u)) / FAT12_SECTOR_SIZE);

    g_Fs.fat_lba = g_Fs.reserved_sectors;
    g_Fs.root_dir_lba = g_Fs.fat_lba + (uint32_t)g_Fs.fat_count * (uint32_t)g_Fs.sectors_per_fat;
    g_Fs.data_lba = g_Fs.root_dir_lba + g_Fs.root_dir_sectors;

    if (g_Fs.data_lba >= g_Fs.total_sectors)
        return 0;

    uint32_t data_sectors = g_Fs.total_sectors - g_Fs.data_lba;
    g_Fs.cluster_count = data_sectors / g_Fs.sectors_per_cluster;

    // FAT type heuristic: FAT12 has < 4085 clusters.
    if (g_Fs.cluster_count >= 4085u)
        return 0;

    g_Fs.fat_bytes = (uint32_t)g_Fs.sectors_per_fat * FAT12_SECTOR_SIZE;
    g_Fs.root_dir_bytes = g_Fs.root_dir_sectors * FAT12_SECTOR_SIZE;

    if (g_Fs.sectors_per_fat > 255u || g_Fs.root_dir_sectors > 255u)
        return 0;

    g_Fs.fat = (uint8_t*)kmalloc(g_Fs.fat_bytes);
    g_Fs.root_dir = (uint8_t*)kmalloc(g_Fs.root_dir_bytes);
    if (!g_Fs.fat || !g_Fs.root_dir)
        goto fail;

    if (!ata_read_sectors(g_Fs.fat_lba, (uint8_t)g_Fs.sectors_per_fat, g_Fs.fat))
        goto fail;

    if (!ata_read_sectors(g_Fs.root_dir_lba, (uint8_t)g_Fs.root_dir_sectors, g_Fs.root_dir))
        goto fail;

    g_Fs.ready = 1;
    return 1;

fail:
    if (g_Fs.fat)
        kfree(g_Fs.fat);
    if (g_Fs.root_dir)
        kfree(g_Fs.root_dir);
    mem_zero(&g_Fs, (uint32_t)sizeof(g_Fs));
    return 0;
}

int fat12_list_root(fat12_file_info_t* out, uint32_t max, uint32_t* out_count)
{
    if (!out_count)
        return 0;

    *out_count = 0;

    if (!g_Fs.ready)
        return 0;

    if (!out || max == 0)
        return 1;

    uint32_t n = 0;
    for (uint32_t i = 0; i < g_Fs.root_entry_count && n < max; i++)
    {
        fat_dirent_t* e = root_entry(i);
        if (e->name[0] == 0x00u)
            break;
        if (!dirent_is_valid_file(e))
            continue;

        name11_to_str(e->name, out[n].name);
        out[n].size = e->file_size;
        n++;
    }

    *out_count = n;
    return 1;
}

int fat12_read_file(const char* name, uint8_t** out_data, uint32_t* out_size)
{
    if (!out_data || !out_size)
        return 0;

    *out_data = 0;
    *out_size = 0;

    if (!g_Fs.ready)
        return 0;

    uint8_t name11[11];
    if (!fat12_format_name11(name, name11))
        return 0;

    uint32_t idx = 0;
    if (!fat12_find_root(name11, &idx))
        return 0;

    fat_dirent_t* e = root_entry(idx);
    if (!dirent_is_valid_file(e))
        return 0;

    uint32_t size = e->file_size;
    uint16_t cluster = e->fst_clus_lo;

    if (size == 0)
    {
        *out_data = 0;
        *out_size = 0;
        return 1;
    }

    if (cluster < 2)
        return 0;

    uint8_t* buf = (uint8_t*)kmalloc(size);
    if (!buf)
        return 0;

    uint32_t remaining = size;
    uint8_t* dst = buf;

    uint8_t sector[FAT12_SECTOR_SIZE];

    for (uint32_t chain_guard = 0; chain_guard < g_Fs.cluster_count + 2u && remaining; chain_guard++)
    {
        if (cluster < 2 || cluster >= FAT12_EOC_MIN)
            break;

        uint32_t lba0 = cluster_lba(cluster);
        for (uint32_t s = 0; s < g_Fs.sectors_per_cluster && remaining; s++)
        {
            if (!ata_read_sectors(lba0 + s, 1, sector))
            {
                kfree(buf);
                return 0;
            }

            uint32_t to_copy = (remaining < FAT12_SECTOR_SIZE) ? remaining : FAT12_SECTOR_SIZE;
            mem_copy(dst, sector, to_copy);
            dst += to_copy;
            remaining -= to_copy;
        }

        if (remaining == 0)
            break;

        cluster = fat12_get(cluster);
    }

    if (remaining != 0)
    {
        kfree(buf);
        return 0;
    }

    *out_data = buf;
    *out_size = size;
    return 1;
}

static int fat12_write_data_exact(uint16_t first_cluster,
                                 uint32_t clusters,
                                 const uint8_t* data,
                                 uint32_t bytes,
                                 uint32_t* bytes_written_out)
{
    if (bytes_written_out)
        *bytes_written_out = 0;

    if (bytes == 0)
        return 1;
    if (!data)
        return 0;
    if (clusters == 0)
        return 0;
    if (first_cluster < 2)
        return 0;

    uint32_t remaining = bytes;
    const uint8_t* src = data;
    uint16_t cluster = first_cluster;

    uint8_t sector[FAT12_SECTOR_SIZE];

    for (uint32_t ci = 0; ci < clusters && remaining; ci++)
    {
        if (cluster < 2 || cluster >= FAT12_EOC_MIN)
            return 0;

        uint32_t lba0 = cluster_lba(cluster);
        for (uint32_t s = 0; s < g_Fs.sectors_per_cluster && remaining; s++)
        {
            uint32_t to_write = (remaining < FAT12_SECTOR_SIZE) ? remaining : FAT12_SECTOR_SIZE;
            mem_zero(sector, FAT12_SECTOR_SIZE);
            mem_copy(sector, src, to_write);

            if (!ata_write_sectors(lba0 + s, 1, sector))
                return 0;

            src += to_write;
            remaining -= to_write;
        }

        cluster = fat12_get(cluster);
    }

    if (bytes_written_out)
        *bytes_written_out = bytes - remaining;

    return 1;
}

int fat12_write_file(const char* name, const uint8_t* data, uint32_t size)
{
    if (!g_Fs.ready)
        return 0;

    uint8_t name11[11];
    if (!fat12_format_name11(name, name11))
        return 0;

    uint32_t entry_idx = 0;
    int exists = fat12_find_root(name11, &entry_idx);
    if (!exists)
    {
        if (!fat12_find_free_root(&entry_idx))
            return 0;

        fat_dirent_t* e = root_entry(entry_idx);
        mem_zero(e, (uint32_t)sizeof(*e));
        mem_copy(e->name, name11, 11u);
        e->attr = FAT_ATTR_ARCHIVE;
        e->fst_clus_lo = 0;
        e->file_size = 0;
    }

    fat_dirent_t* e = root_entry(entry_idx);
    uint16_t old_first = e->fst_clus_lo;

    // Empty file: free existing chain and update dir entry.
    if (size == 0)
    {
        if (old_first >= 2)
            fat12_free_chain(old_first);

        e->fst_clus_lo = 0;
        e->file_size = 0;
        return fat12_sync();
    }

    uint32_t cluster_bytes = (uint32_t)g_Fs.sectors_per_cluster * FAT12_SECTOR_SIZE;
    if (cluster_bytes == 0)
        return 0;

    uint32_t clusters_needed = (size + cluster_bytes - 1u) / cluster_bytes;
    if (clusters_needed == 0)
        return 0;

    // Measure existing chain length (if any).
    uint32_t old_count = 0;
    uint16_t last_old = 0;
    uint16_t cur = old_first;
    for (uint32_t guard = 0; guard < g_Fs.cluster_count + 2u; guard++)
    {
        if (cur < 2 || cur >= FAT12_EOC_MIN)
            break;
        old_count++;
        last_old = cur;
        cur = fat12_get(cur);
    }

    uint32_t use_old = (old_count < clusters_needed) ? old_count : clusters_needed;
    uint32_t need_new = clusters_needed - use_old;

    // Allocate new cluster chain for the extra part (not linked yet).
    uint16_t new_first = 0;
    if (need_new)
    {
        uint16_t first = 0;
        uint16_t prev = 0;

        for (uint32_t i = 0; i < need_new; i++)
        {
            uint16_t c = fat12_alloc_cluster();
            if (!c)
            {
                if (first)
                    fat12_free_chain(first);
                return 0;
            }

            if (prev)
                fat12_set(prev, c);
            else
                first = c;
            prev = c;
            fat12_set(c, FAT12_EOC);
        }

        new_first = first;
    }

    // Determine file start cluster.
    uint16_t start = (use_old > 0) ? old_first : new_first;
    if (start < 2)
        return 0;

    // Write data into [old chain prefix] then [new chain] (if any).
    uint32_t written = 0;
    if (use_old)
    {
        if (!fat12_write_data_exact(old_first, use_old, data, size, &written))
            return 0;
    }

    if (written < size)
    {
        uint32_t written2 = 0;
        if (!fat12_write_data_exact(new_first,
                                    need_new,
                                    data + written,
                                    size - written,
                                    &written2))
            return 0;
        written += written2;
    }

    if (written != size)
        return 0;

    // Update FAT chain links and free any leftover old clusters.
    if (use_old == 0)
    {
        // New file.
        e->fst_clus_lo = start;
    }
    else
    {
        // Link old chain to new chain (if we extended).
        if (need_new)
            fat12_set(last_old, new_first);

        // If we shrank, truncate and free the tail.
        if (old_count > clusters_needed)
        {
            // Find last needed cluster and the first cluster after it.
            uint16_t last_needed = old_first;
            for (uint32_t i = 1; i < clusters_needed; i++)
                last_needed = fat12_get(last_needed);

            uint16_t after = fat12_get(last_needed);
            fat12_set(last_needed, FAT12_EOC);
            if (after >= 2 && after < FAT12_EOC_MIN)
                fat12_free_chain(after);
        }

        e->fst_clus_lo = old_first;
    }

    e->file_size = size;

    return fat12_sync();
}
