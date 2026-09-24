/* ========================================================================
   $File: using.cpp $
   $Date: September 24 2026 01:09 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

template <typename T, int count>
struct array_t
{
    T  *items;
    int count;
};

using item_array_t = array_t<int, count>;
struct items {
    item_array_t items;
};

using namespace std;

int
main(void)
{
    cout << "EW this syntax is so bad lmfaoooooooooo" << endl;
}
