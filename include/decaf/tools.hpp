#ifndef DECAF_TOOLS
#define DECAF_TOOLS

#include <stdio.h>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <decaf/types.hpp>

using namespace std;

namespace decaf
{

void all_err(int err_code);
Decomposition stringToDecomposition(std::string name);
TransportMethod stringToTransportMethod(std::string method);
Check_level stringToCheckLevel(string check);

} //decaf

#endif
