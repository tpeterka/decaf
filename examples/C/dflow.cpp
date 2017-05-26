#include <decaf/decaf.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <stdio.h>

using namespace decaf;
// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        fprintf(stdout, "Processing dflow\n");
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"
