#include <stdio.h>
#include <stdlib.h>
#include <bredala/C/bredala.h>

#define SIZE 10

int main()
{
    printf("Preparation of the data\n");
    int* array = (int*)(malloc(SIZE * sizeof(int)));
    unsigned int i;
    for(i = 0; i < SIZE; i++)
        array[i] = i;
    printf("Initial array : [ ");
    for(i = 0; i < SIZE; i++)
        printf("%i ",array[i]);
    printf("]\n");

    int single_data = 256;
    printf("Initial simple data : %i\n", single_data);

    printf("Creation of the container...");
    bca_constructdata container = bca_create_constructdata();
    printf("OK.\n");
    printf("Creation of the array field...");
    bca_field field_array = bca_create_arrayfield(array, bca_INT, SIZE, 1, SIZE, false);
    printf("OK.\n");
    printf("Creation of the simple field...");
    bca_field field_simple = bca_create_simplefield(&single_data, bca_INT);
    printf("OK.\n");

    printf("Append simple field in the container...");
    if(bca_append_field(container, "simple", field_simple, bca_NOFLAG,bca_SHARED,bca_SPLIT_DEFAULT,bca_MERGE_DEFAULT))
        printf("OK\n");
    else
        printf("FAILED\n");
    printf("Append array field in the container...");
    if(bca_append_field(container, "array", field_array, bca_NOFLAG,bca_PRIVATE,bca_SPLIT_DEFAULT,bca_MERGE_DEFAULT))
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
    printf("Size of the array : %zu\n", size_new_array);
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

    printf("Splitting the data model...");
    int ranges[3];
    ranges[0] = 3;
    ranges[1] = 3;
    ranges[2] = 4;

    bca_constructdata results[3];
    if(bca_split_by_range(container,3,ranges,results))
        printf("Ok\n");
    else
        printf("FAILED\n");

    for(i = 0; i < 3; i++)
    {
        int* sub_array;
        size_t size_sub_array;
        bca_field get_field_array = bca_get_arrayfield(results[i], "array", bca_INT);
        if(get_field_array == NULL)
        {
            printf("ERROR : Can not retrive the field array\n");
            continue;
        }
        sub_array = bca_get_array(get_field_array, bca_INT, &size_sub_array);
        if(sub_array == NULL)
        {
            printf("ERROR : Can not retrive the array from the field\n");
            continue;
        }
        printf("Subarray %i : [ ", i);
        unsigned int j;
        for(j = 0; j < size_sub_array; j++)
            printf("%i ", sub_array[j]);
        printf("]\n");
        bca_free_field(get_field_array);
    }

    printf("Merging the data models...");
    bca_constructdata merged_container = bca_create_constructdata();
    if(!bca_merge_constructdata(merged_container, results[0]))
        printf("FAILED\n");
    else if(!bca_merge_constructdata(merged_container, results[1]))
        printf("FAILED\n");
    else if(!bca_merge_constructdata(merged_container, results[2]))
        printf("FAILED\n");
    else
        printf("OK\n");

    bca_field arrayfield_merged = bca_get_arrayfield(results[0],"array", bca_INT);
    if(arrayfield_merged == NULL)
        printf("ERROR : unable to retrieve the field \"array\"\n");
    int* array_merged;
    size_t size_merged_array;
    array_merged = bca_get_array(arrayfield_merged, bca_INT, &size_merged_array);
    if(array_merged == NULL)
        printf("ERROR : Can not retrieve the array from the merged field\n");
    printf("Merged array : [ ");
    for(i = 0; i < size_merged_array; i++)
        printf("%i ", array_merged[i]);
    printf("]\n");


    printf("Cleaning...");
    bca_free_field(get_field_simple);
    bca_free_field(get_field_array);
    bca_free_field(field_simple);
    bca_free_field(field_array);
    bca_free_field(arrayfield_merged);
    bca_free_constructdata(container);
    bca_free_constructdata(merged_container);
    for(i = 0; i < 3; i++)
    {
        bca_free_constructdata(results[i]);
    }
    free(array);
    printf("OK\n");
}
