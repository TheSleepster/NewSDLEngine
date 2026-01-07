#if !defined(C_NEW_HASH_TABLE_H)
/* ========================================================================
   $File: c_new_hash_table.h $
   $Date: January 06 2026 06:26 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_NEW_HASH_TABLE_H
#include <c_types.h>
#include <c_base.h>
#include <c_string.h>

typedef struct hash_table_header
{
    u32       max_entries;
    u32       data_type_size;
    u32       flags;
    u32       __reserved0;
    u32       __reserved1;
    
    string_t *keys;
}hash_table_header_t;
StaticAssert(sizeof(hash_table_header_t) % 16 == 0, "Hash table header must be 16 byte aligned...\n");

#define HashTable(type, max_entries) \
struct {                             \
    hash_table_header_t header;      \
    type               *data;        \
} 

#define c_hash_table_init

void
c_hash_table_init(void *table);

#endif // C_NEW_HASH_TABLE_H

