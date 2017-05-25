#ifndef MANALA_TOOLS_H
#define MANALA_TOOLS_H

#include <stdio.h>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <manala/types.h>

StreamPolicy stringToStreamPolicy(std::string name);

StorageType stringToStoragePolicy(std::string name);

StorageCollectionPolicy stringToStorageCollectionPolicy(std::string name);

FramePolicyManagment stringToFramePolicyManagment(std::string name);

#endif
