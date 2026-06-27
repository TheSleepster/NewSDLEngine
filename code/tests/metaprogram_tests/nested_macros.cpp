#define KB(x) (x * 1000)
#define MB(x) (KB(x) * 1000)
#define GB(x) (MB(x) * 1000)

constexpr u32 ELEMENT_SIZE = 10;

#define Min(A, B) (A < B ? A : B)
#define Max(A, B) (A > B ? A : B)

struct sizes
{
    u32 size[MB(10)];
    u32 item;
};