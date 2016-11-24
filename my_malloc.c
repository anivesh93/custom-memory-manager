#include "my_malloc.h"

void *memory_resource_void;
char *memory_resource;

const int page_size = 4096;
const int num_of_pages = (8 * 1024 * 1024) / 4096;

int Gthread_id = 0;

int num_of_page_headers;
int page_headers_per_page;
int pages_used_by_page_headers;
int total_usable_pages;
int remaining_pages;

void my_malloc_init()
{
    num_of_page_headers = num_of_pages;
    page_headers_per_page = page_size / sizeof(page_header);
    printf("page headers per page: %d\n", page_headers_per_page);
    pages_used_by_page_headers = num_of_page_headers/page_headers_per_page;
    printf("pages used by page headers: %d\n", pages_used_by_page_headers);
    total_usable_pages = num_of_pages - pages_used_by_page_headers;
    printf("total usable pages: %d\n", total_usable_pages);
    remaining_pages = total_usable_pages;
    
    posix_memalign(&memory_resource_void, 4096, num_of_pages*page_size);
    memory_resource = (char*) memory_resource_void;
    
    printf("Physical memory starts from %p\n", memory_resource);
    printf("page memory starts from %p\n", memory_resource + (pages_used_by_page_headers * page_size));

    int ctr = 0;
    int i;
    for (i = 0; i < pages_used_by_page_headers; i++) 
    {
        
        char* page_ptr = (char*)(memory_resource + i * page_size);
        int j;
        for (j = 0; j < page_headers_per_page; j++) 
        {
            if (ctr < 2048)
            {
                page_header* ptr = (page_header*)(page_ptr + j * (sizeof(page_header)));
                ptr -> is_allocated = 0;
                ptr -> page_num = ctr++;
                ptr -> offset = -1;
            }
            else
                break;
        }
    }
}




void* myallocate(int num_of_bytes, char* file_name, int line_number, int thread_id)
{
    //mprotect(memory_resource, num_of_pages * page_size, PROT_READ | PROT_WRITE);
    int num_of_pages_req = num_of_bytes/page_size + 1;
    int size =  num_of_bytes;
    
    if(num_of_pages_req > remaining_pages)
        return NULL;
    
    int first_page_found = -1;
    page_header* ptr = NULL;
    page_header* first_ptr = NULL;
    void* page_ptr = NULL;
    
    while (size > 0) 
    {
        int old_offset = -1;
        printf("get a header for page in which data will be written\n");
        ptr = get_header_for_next_usable_page(thread_id, &size, &old_offset);
        printf("got a header for page in which data will be written\n");
        printf("first frame found? %d\n", first_page_found);
        if(first_page_found == -1) 
        {
            first_ptr = ptr;
            first_page_found = 1;
            page_ptr = memory_resource + (pages_used_by_page_headers + first_ptr->thread_header_num)*page_size;
            page_ptr += old_offset + 1;
        }
        
    }
    //mprotect(memory_resource, num_of_pages * page_size, PROT_NONE);
    return page_ptr;
}



page_header* get_header_for_next_usable_page(int thread_id, int *num_of_bytes, int *old_offset)
{
    
    page_header* last_header_for_current_thread = get_last_header_for_current_thread(thread_id);
    page_header* retval = NULL;
    int i;
    
    if(last_header_for_current_thread != NULL && last_header_for_current_thread->offset != -1)
    {
        int space_on_page = page_size - last_header_for_current_thread->offset - 1;
        *old_offset = last_header_for_current_thread->offset;
        if(*num_of_bytes >= space_on_page)
        {
            *num_of_bytes -= space_on_page;
            last_header_for_current_thread->offset = -1;
        }
        else
        {
            last_header_for_current_thread->offset += *num_of_bytes;
            *num_of_bytes = 0;
        }
        retval = last_header_for_current_thread;
        return retval;
    }
    else
    {
        int ctr = 0;
        for (i = 0; i < pages_used_by_page_headers; i++) 
        {
            char* page_ptr = (char*)(memory_resource + i*page_size);
            int j;
            for (j = 0; j < page_headers_per_page; j++) 
            {
                if (ctr > 2047)
                    break;
                ctr++;

                if (i == 0 && j < 10)
                    continue;

                page_header* ptr = (page_header*)(page_ptr + j*(sizeof(page_header)));
            
                if(ptr -> is_allocated == 0)
                {
                    printf("page number of this page: %d\n", ptr->page_num);
                    ptr->is_allocated = 1;
                    ptr->thread_id = thread_id;
                    
                    if(last_header_for_current_thread != NULL) 
                        ptr->thread_header_num = (last_header_for_current_thread->thread_header_num)+1;
                    else
                        ptr->thread_header_num = 0;

                    printf("thread_header_num: %d\n", ptr->thread_header_num);

                    *old_offset = ptr->offset;
                    
                    printf("num_of_bytes: %d\n", *num_of_bytes);
                    if(*num_of_bytes >= page_size)
                    {
                        ptr->offset = -1;
                        *num_of_bytes -= page_size;
                    }
                    else
                    {
                        ptr->offset += *num_of_bytes;
                        *num_of_bytes = 0;
                    }
                    printf("ptr->offset: %d, oldOffset: %d \n", ptr->offset, *old_offset);
                    
                    retval = ptr;
                    remaining_pages--;
                    printf("remaining_pages: %d\n", remaining_pages);
                    
                    return retval;
                }
            }
        }
    }
    
    return retval;
}

page_header* get_last_header_for_current_thread(int thread_id)
{
    page_header *retval = NULL;
    
    int i, ctr = 0;
    for (i = 0; i < pages_used_by_page_headers; i++) 
    {
        
        char *page_ptr = (char*)(memory_resource + i*page_size);
        int j;
        for (j = 0; j < page_headers_per_page; j++) 
        {
            if (ctr > 2047)
                break;
            ctr++;
            //printf("counter: %d\n", ctr);


            page_header *ptr = (page_header*)(page_ptr + j*(sizeof(page_header)));
            //printf("page number: %d\n", ptr -> page_num);

            if(ptr -> is_allocated == 1 && ptr -> thread_id == thread_id)
            {
                if(retval == NULL || (retval->thread_header_num < ptr->thread_header_num))
                    retval = ptr;
            }
            
        }
        
    }
    return retval; 
}


int main() 
{

    my_malloc_init();
    printf("init done\n");
    //mprotectFunc(getPhyMem(),8388608,PROT_NONE);
    mprotect(memory_resource, 2048 * 4096, PROT_NONE);
    char* test = myallocate(4099, __FILE__, __LINE__ , 1);
    Gthread_id = 1;
    *(test) = 'a';
    *(test+1) = 'b';
    *(test+2) = 'c';
    *(test+4098) = 'r';
    /*
    printf("test is: %p %c%c%c\n",test,*(test),*(test+1),*(test+4098));
    
    char* test1 = myallocate(4093, __FILE__, __LINE__ , 1);
    Gthread_id=1;
    mprotect(memory_resource, 2048 * 4096, PROT_NONE);
    //printf("test is: %p\n",test);
    *(test1) = 'd';
    *(test1+1) = 'e';
    *(test1+2) = 'f';
    printf("test is:%p %c%c%c\n",test1,*(test1),*(test1+1),*(test1+2));
    
    char* test2 = myallocate(3, __FILE__, __LINE__ , 2);
    mprotect(memory_resource, 2048 * 4096, PROT_NONE);
    Gthread_id = 2;
    *(test2) = 't';
    *(test2+1) = 'u';
    *(test2+2) = 'v';
    printf("test is: %p %c%c%c\n",test2,*(test2),*(test2+1),*(test2+2));
    */
    return 0;
}








