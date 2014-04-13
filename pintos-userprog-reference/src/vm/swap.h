
void swap_init();
int write_page_to_swap(void *kpage);
void read_page_from_swap(void *kpage,int location);
void delete_page(int sector);
