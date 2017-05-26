#include <bredala/data_model/maptools.h>

using namespace decaf;

ConstructTypeFlag& decaf::getFlag(datafield& field)
{
    return std::get<0>(field);
}

ConstructTypeScope& decaf::getScope(datafield& field)
{
    return std::get<1>(field);
}

int& decaf::getNbItemsField(datafield& field)
{
    return std::get<2>(field);
}

std::shared_ptr<BaseConstructData>& decaf::getBaseData(datafield& field)
{
    return std::get<3>(field);
}

ConstructTypeSplitPolicy& decaf::getSplitPolicy(datafield& field)
{
    return std::get<4>(field);
}

ConstructTypeMergePolicy& decaf::getMergePolicy(datafield& field)
{
    return std::get<5>(field);
}
