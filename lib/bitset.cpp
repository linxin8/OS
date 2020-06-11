#include "lib/bitset.h"
#include "lib/debug.h"
#include "lib/string.h"

// void bitset_init(Bitset* bitset)
// {
//     ASSERT(bitset!=NULLPTR);
//     ASSERT(bitset->data!=NULLPTR);
//     memset(bitset->data,0,bitset->length);
// }

// bool bitset_test(Bitset* bitset, size_t pos)
// {
//     size_t byte_index=pos/8;
//     size_t bit_index=pos%8;
//     return bitset->data[byte_index]&(1<<bit_index);
// }

// Bitset::Bitset(uint32_t bit_size)
// {

// }