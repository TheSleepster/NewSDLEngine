namespace GameData {
    struct item 
    {
        const char *name;
        short       item_data[256];

        void get_item(char *name);
    };

    void item::get_item(char *name)
    {
    }
}