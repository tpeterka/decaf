#ifndef MAPTOOLS
#define MAPTOOLS

#include <map>
#include <memory>


namespace decaf {

enum ConstructTypeFlag {
    DECAF_NOFLAG = 0x0,     // No specific information on the data field
    DECAF_NBITEM = 0x1,     // Field represents the number of item in the collection
    DECAF_POS = 0x2,  // Field that can be used as a key for the ZCurve (position)
    DECAF_MORTON = 0x4 // Field that can be used as the index for the ZCurve (hilbert code)
};

enum ConstructTypeScope {
    DECAF_SHARED = 0x0,     // This value is the same for all the items in the collection
    DECAF_PRIVATE = 0x1,    // Different values for each items in the collection
    DECAF_SYSTEM = 0x2,     // This field is not a user data
};

enum ConstructTypeSplitPolicy {
    DECAF_SPLIT_DEFAULT = 0x0,      // Call the split fonction of the data object
    DECAF_SPLIT_KEEP_VALUE = 0x1,   // Keep the same values for each split
    DECAF_SPLIT_MINUS_NBITEM = 0x2, // Withdraw the number of items to the current value
    DECAF_SPLIT_SEGMENTED = 0x4,
};

enum ConstructTypeMergePolicy {
    DECAF_MERGE_DEFAULT = 0x0,        // Call the split fonction of the data object
    DECAF_MERGE_FIRST_VALUE = 0x1,    // Keep the same values for each split
    DECAF_MERGE_ADD_VALUE = 0x2,      // Add the values
    DECAF_MERGE_APPEND_VALUES = 0x4,  // Append the values into the current object
    DECAF_MERGE_BBOX_POS = 0x8,       // Compute the bounding box from the field pos
};

class BaseConstructData; // Declared later

//Structure for ConstructType
typedef std::tuple<ConstructTypeFlag, ConstructTypeScope,
    int, std::shared_ptr<BaseConstructData>,
    ConstructTypeSplitPolicy, ConstructTypeMergePolicy> datafield;

typedef std::shared_ptr<std::map<std::string, datafield> > mapConstruct;

ConstructTypeFlag& getFlag(datafield& field);

ConstructTypeScope& getScope(datafield& field);

int& getNbItemsField(datafield& field);

std::shared_ptr<BaseConstructData>& getBaseData(datafield& field);

ConstructTypeSplitPolicy& getSplitPolicy(datafield& field);

ConstructTypeMergePolicy& getMergePolicy(datafield& field);

} // namespace

#endif
