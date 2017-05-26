#ifndef MSG_TOOLS
#define MSG_TOOLS

#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>
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


static
int
get_iteration(pConstructData data)
{
	shared_ptr<SimpleConstructData<int> > ptr =
	        data->getTypedData<SimpleConstructData<int> >("decaf_iteration");
	if(ptr){
		return ptr->getData();
	}
	return -1;
}

// puts an iteration number into a container
static
void
set_iteration(pConstructData out_data, int iteration)
{
	if(out_data->hasData("decaf_iteration")){
		if(get_iteration(out_data) == iteration){ // The correct iteration is already set
			return;
		}
		// TODO remove this cout warning, was for debuging purpose
		std::cout << "WARNING: iteration already set with different value: "<< get_iteration(out_data) << " " << iteration << std::endl;
		out_data->removeData("decaf_iteration");
	}
	shared_ptr<SimpleConstructData<int> > data  =
	    make_shared<SimpleConstructData<int> >(iteration);
	out_data->appendData(string("decaf_iteration"), data,
	                     DECAF_NOFLAG, DECAF_SYSTEM,
	                     DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
}


} // namespace msgtools

} // namespace decaf

#endif
