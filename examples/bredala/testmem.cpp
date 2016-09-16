#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/arrayfield.hpp>
#include <memory>
#include <vector>
#include <iostream>

#define SIZE 1000

using namespace decaf;
using namespace std;

int main()
{
    int* array = new int[SIZE];
    pConstructData container;
    ArrayFieldi field(array, SIZE, 1, SIZE, false);

    container->appendData("array", field, DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    field.reset();
    vector<int> ranges = {300,300,400};

    vector<shared_ptr<BaseData> > chunks = container->split(ranges);

    pConstructData merged;
    merged->merge(chunks[0]);
    merged->merge(chunks[1]);
    merged->merge(chunks[2]);

    delete [] array;
}
