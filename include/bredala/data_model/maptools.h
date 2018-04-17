#ifndef MAPTOOLS
#define MAPTOOLS

#include <map>
#include <memory>


namespace decaf {

/** This flag indicates the type of a field:
 * @param DECAF_NOFLAG: 	No specific information on the data field
 * @param DECAF_NBITEM: 	(deprecated) Field represents the number of items in the collection
 * @param DECAF_POS: 		Field that can be used as a key for the ZCurve (position)
 * @param DECAF_MORTON: 	Field that can be used as the index for the ZCurve (Morton code)
 */

enum ConstructTypeFlag {
    DECAF_NOFLAG = 0x0,     // No specific information on the data field
    DECAF_NBITEM = 0x1,     // Field represents the number of item in the collection
    DECAF_POS = 0x2,  // Field that can be used as a key for the ZCurve (position)
    DECAF_MORTON = 0x4 // Field that can be used as the index for the ZCurve (hilbert code)
};

/** This flag indicates the scope of a field:
 * @param DECAF_SHARED: 	The value of the field shared by all the items in the collection (analogous to global variable)
 * @param DECAF_PRIVATE: 	Each (split) group have their own values for the field in the collection (analogous to local variable)
 * @param DECAF_SYSTEM: 	Field that is part of a system data. Different than user fields (that can be split), system fields need to be sent to every destination.
 */

enum ConstructTypeScope {
    DECAF_SHARED = 0x0,     // This value is the same for all the items in the collection
    DECAF_PRIVATE = 0x1,    // Different values for each items in the collection
    DECAF_SYSTEM = 0x2,     // This field is not a user data
};

/** This flag indicates the split policy for a field:
 * @param DECAF_SPLIT_DEFAULT: 		Applies the default split function of the data type (Simple->DECAF_MERGE_FIRST_VALUE, Array/Vector->DECAF_MERGE_APPEND_VALUES)
 * @param DECAF_SPLIT_KEEP_VALUE: 	Keeps the same values when splitting.
 * @param DECAF_SPLIT_MINUS_NBITEM: 	Assign the value of number of items in the split group (Mostly used for variables as number of particles which indicate the size of a split group.)
 * @param DECAF_SPLIT_SEGMENTED: 	Not supported currently, considered for the array data type.
 */

enum ConstructTypeSplitPolicy {
    DECAF_SPLIT_DEFAULT = 0x0,      // Call the split fonction of the data object
    DECAF_SPLIT_KEEP_VALUE = 0x1,   // Keep the same values for each split
    DECAF_SPLIT_MINUS_NBITEM = 0x2, // Withdraw the number of items to the current value
    DECAF_SPLIT_SEGMENTED = 0x4,
};

/** This flag indicates the merge policy for a field:
 * @param DECAF_MERGE_DEFAULT: 	     Applies the default merge function of the data type (Simple->DECAF_MERGE_FIRST_VALUE, Array/Vector->DECAF_MERGE_APPEND_VALUES)
 * @param DECAF_MERGE_FIRST_VALUE:   Keeps the same values when merging.
 * @param DECAF_MERGE_ADD_VALUE:     Adds the values (Mostly used for variables as number of particles which indicate the size of a split group.)
 * @param DECAF_MERGE_APPEND_VALUES: Appends the values into the current object
 * @param DECAF_MERGE_BBOX_POS:      Computes the bounding box from the field pos
 */

enum ConstructTypeMergePolicy {
    DECAF_MERGE_DEFAULT = 0x0,        // Call the split fonction of the data object
    DECAF_MERGE_FIRST_VALUE = 0x1,    // Keep the same values for each split
    DECAF_MERGE_ADD_VALUE = 0x2,      // Add the values
    DECAF_MERGE_APPEND_VALUES = 0x4,  // Append the values into the current object
    DECAF_MERGE_BBOX_POS = 0x8,       // Compute the bounding box from the field pos
};

class BaseConstructData; // Declared later

//Structure for ConstructType
//! field definition with its associated flags
typedef std::tuple<ConstructTypeFlag, ConstructTypeScope,
    int, std::shared_ptr<BaseConstructData>,
    ConstructTypeSplitPolicy, ConstructTypeMergePolicy> datafield;

//! data model: collection of fields
typedef std::shared_ptr<std::map<std::string, datafield> > mapConstruct;

//! returns the flag which indicates the type of a field
ConstructTypeFlag& getFlag(datafield& field);

//! returns the scope of a field
ConstructTypeScope& getScope(datafield& field);

//! returns the number of semantic items
int& getNbItemsField(datafield& field);

std::shared_ptr<BaseConstructData>& getBaseData(datafield& field);

//! returns the split policy
ConstructTypeSplitPolicy& getSplitPolicy(datafield& field);

//! returns the merge policy
ConstructTypeMergePolicy& getMergePolicy(datafield& field);

} // namespace

#endif
