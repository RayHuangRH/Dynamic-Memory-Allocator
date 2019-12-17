#include <stdio.h>
#include <float.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    void *ptr1 = sf_malloc(50);
    sf_show_heap();
    ptr1 = sf_realloc(ptr1, 100);
    sf_show_heap();
    ptr1 = sf_realloc(ptr1, 5000);
    sf_show_heap();
    ptr1 = sf_realloc(ptr1, 10);
    sf_show_heap();
    ptr1 = sf_realloc(ptr1, 100);
    sf_show_heap();
    ptr1 = sf_realloc(ptr1, 700);
    sf_show_heap();
    // double* ptr = sf_malloc(sizeof(double));

    // *ptr = 320320320e-320;

    // printf("%f\n", *ptr);

    // sf_free(ptr);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
