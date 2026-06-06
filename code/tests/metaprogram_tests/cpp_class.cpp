/* ========================================================================
   $File: cpp_class.cpp $
   $Date: May 24 2026 09:26 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

struct CPP_test_t
{
public:
    int apples;
    int oranges;

    void get_apples(void);

private:
    int some_secret_item;
};

class class_test_t : public CPP_test_t
{
public:
    float other_apples;
private:
    void some_stupid_getter(void *data);
    void some_expanded_setter(int *data)
    {
        this->apples = *data;   
    }

    float hidden_apples;
};
