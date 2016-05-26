#include <stdio.h>
#include <decaf/C/breadala.h>

#define SIZE 10

int main()
{
    printf("Preparation of the data\n");
    int* array = (int*)(malloc(SIZE * sizeof(int)));
    int i;
    for(i = 0; i < SIZE; i++)
        array[i] = i;
    printf("Initial array : [ ");
    for(i = 0; i < SIZE; i++)
        printf("%i ",array[i]);
    printf("]\n");

    int single_data = 256;
    printf("Initial simple data : %i\n", single_data);

    printf("Creation of the container...");
    bca_constructdata container = bca_new_constructdata();
    printf("OK.\n");
    printf("Creation of the array field...");
    bca_field field_array = bca_create_arrayfield(array, bca_INT, SIZE, 1, SIZE, false);
    printf("OK.\n");
    printf("Creation of the simple field...");
    bca_field field_simple = bca_create_simplefield(&single_data, bca_INT);
    printf("OK.\n");

    printf("Append array field in the container...");
    if(bca_append_field(container, "array", field_array, bca_NOFLAG,bca_PRIVATE,bca_SPLIT_DEFAULT,bca_MERGE_DEFAULT))
        printf("OK\n");
    else
        printf("FAILED\n");
    printf("Append simple field in the container...");
    if(bca_append_field(container, "simple", field_simple, bca_NOFLAG,bca_PRIVATE,bca_SPLIT_DEFAULT,bca_MERGE_DEFAULT))
        printf("OK\n");
    else
        printf("FAILED\n");
    printf("Get array field form container...");
    bca_field get_field_array = bca_get_arrayfield(container, "array", bca_INT);
    if(get_field_array != NULL)
        printf("OK\n");
    else
        printf("FAILED\n");
    printf("Get simple field form container...");
    bca_field get_field_simple = bca_get_simplefield(container, "simple", bca_INT);
    if(get_field_simple != NULL)
        printf("OK\n");
    else
        printf("FAILED\n");

    int* new_array = NULL;
    size_t size_new_array;
    printf("Access to the array...");
    new_array = bca_get_array(get_field_array, bca_INT, &size_new_array);
    if(new_array != NULL)
        printf("OK\n");
    else
        printf("FAILED\n");
    printf("Size of the array : %i\n", size_new_array);
    printf("Final array : [ ");
    for(i = 0; i < size_new_array; i++)
        printf("%i ",new_array[i]);
    printf("]\n");

    int new_single;
    printf("Access to the simple data...");
    if(bca_get_data(get_field_simple, bca_INT, &new_single))
        printf("OK\n");
    else
        printf("FAILED\n");
    printf("Final simple data : %i\n", new_single);

    printf("Cleaning...");
    bca_free_field(get_field_simple);
    bca_free_field(get_field_array);
    bca_free_field(field_simple);
    bca_free_field(field_array);
    bca_free_constructdata(container);
    free(array);
    printf("Ok\n");
}
