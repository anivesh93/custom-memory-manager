#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>

typedef struct {
    int is_allocated;
    int thread_id;
    int thread_header_num;
    int page_num;
    int offset;
}page_header;

extern int Gthread_id;

void my_malloc_init();

void* myallocate(int size, char* file_name, int line_number, int ThreadReq);

page_header* get_header_for_next_usable_page(int thread_id, int *num_of_bytes, int *old_offset);

page_header* get_last_header_for_current_thread(int thread_id);



