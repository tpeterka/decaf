#ifndef MSG_TOOLS
#define MSG_TOOLS

#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/simplefield.hpp>
#include <memory>
#include <string>

using namespace std;

namespace decaf {

namespace msgtools {

// sets a quit message into a container; caller still needs to send the message
static
void
set_quit(pConstructData out_data)   // output message
    {
        shared_ptr<SimpleConstructData<int> > data  =
            make_shared<SimpleConstructData<int> >(1);
        out_data->appendData(string("decaf_quit"), data,
                             DECAF_NOFLAG, DECAF_SYSTEM,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
        out_data->setSystem(true);
    }

// tests whether a message is a quit command
static
bool
test_quit(pConstructData in_data)   // input message
{
    return in_data->hasData(string("decaf_quit"));
}

} // namespace msgtools

} // namespace decaf

#endif
