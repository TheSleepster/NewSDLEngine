#define KB(x) (x * 1000)
#define MB(x) (KB(x) * 1000)
#define GB(x) (MB(x) * 1000)

constexpr u32 ELEMENT_SIZE = 10;

struct sizes
{
    u32 size[ELEMENT_SIZE];
    u32 item;
}