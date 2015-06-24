#include <decaf/data_model/constructtype.h>

using namespace decaf;

decaf::
ConstructData::ConstructData() : BaseData(), nbFields_(0), bZCurveIndex_(false), zCurveIndex_(NULL),
        bZCurveKey_(false), zCurveKey_(NULL)
{
    container_ = std::make_shared<std::map<std::string, datafield> >();
    data_ = static_pointer_cast<void>(container_);
}


bool
decaf::
ConstructData::appendData(std::string name,
                std::shared_ptr<BaseConstructData>  data,
                ConstructTypeFlag flags,
                ConstructTypeScope scope,
                ConstructTypeSplitPolicy splitFlag,
                ConstructTypeMergePolicy mergeFlag)
{
    std::pair<std::map<std::string, datafield>::iterator,bool> ret;
    datafield newEntry = make_tuple(flags, scope, data->getNbItems(), data, splitFlag, mergeFlag);
    ret = container_->insert(std::pair<std::string, datafield>(name, newEntry));

    return ret.second && updateMetaData();
}

int
decaf::
ConstructData::getNbFields()
{
    return nbFields_;
}

std::shared_ptr<std::map<std::string, datafield> >
decaf::
ConstructData::getMap()
{
    return container_;
}

void
decaf::
ConstructData::printKeys()
{
    std::cout<<"Current state of the map : "<<std::endl;
    for(std::map<std::string, datafield>::iterator it = container_->begin();
        it != container_->end(); it++)
    {
        std::cout<<"Key : "<<it->first<<", nbItems : "<<std::get<2>(it->second)<<std::endl;
    }
    std::cout<<"End of display of the map"<<std::endl;

}

bool
decaf::
ConstructData::hasZCurveKey()
{
    return bZCurveKey_;
}

const float*
decaf::
ConstructData::getZCurveKey(int *nbItems)
{
    if(!bZCurveKey_ || !zCurveKey_){
        std::cout<<"ERROR : The ZCurve field is empty."<<std::endl;
        return NULL;
    }

    std::shared_ptr<VectorConstructData<float> > field =
            dynamic_pointer_cast<VectorConstructData<float> >(zCurveKey_);
    if(!field){
        std::cout<<"ERROR : The field with the ZCURVE flag is not of type VectorConstructData<float>"<<std::endl;
        return NULL;
    }

    *nbItems = field->getNbItems();
    return &(field->getVector()[0]);
}

bool
decaf::
ConstructData::hasZCurveIndex()
{
    return bZCurveIndex_;
}

const unsigned int*
decaf::
ConstructData::getZCurveIndex(int *nbItems)
{
    if(!bZCurveIndex_ || !zCurveIndex_)
        return NULL;

    std::shared_ptr<VectorConstructData<unsigned int> > field =
            dynamic_pointer_cast<VectorConstructData<unsigned int> >(zCurveIndex_);
    if(!field){
        std::cout<<"ERROR : The field with the ZCURVE flag is not of type VectorConstructData<float>"<<std::endl;
        return NULL;
    }

    *nbItems = field->getNbItems();
    return &(field->getVector()[0]);
}

bool
decaf::
ConstructData::isSplitable()
{
    return nbItems_ > 1;
}

std::vector< std::shared_ptr<BaseData> >
decaf::
ConstructData::split(
        const std::vector<int>& range)
{
    std::vector< std::shared_ptr<BaseData> > result;
    for(unsigned int i = 0; i < range.size(); i++)
        result.push_back(std::make_shared<ConstructData>());

    //Sanity check
    int totalRange = 0;
    for(unsigned int i = 0; i < range.size(); i++)
        totalRange+= range.at(i);
    if(totalRange != getNbItems()){
        std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                 <<") does not match the number of items of the object ("
                 <<getNbItems()<<")"<<std::endl;
        return result;
    }

    for(std::map<std::string, datafield>::iterator it = container_->begin();
        it != container_->end(); it++)
    {
        // Splitting the current field
        std::vector<std::shared_ptr<BaseConstructData> > splitFields;
        splitFields = std::get<3>(it->second)->split(range, std::get<4>(it->second));

        // Inserting the splitted field into the splitted results
        if(splitFields.size() != result.size())
        {
            std::cout<<"ERROR : A field was not splited properly."
                    <<" The number of chunks does not match the expected number of chunks"<<std::endl;
            // Cleaning the result to avoid corrupt data
            result.clear();

            return result;
        }

        //Adding the splitted results into the splitted maps
        for(unsigned int i = 0; i < result.size(); i++)
        {
            std::shared_ptr<ConstructData> construct = dynamic_pointer_cast<ConstructData>(result.at(i));
            construct->appendData(it->first,
                                     splitFields.at(i),
                                     std::get<0>(it->second),
                                     std::get<1>(it->second),
                                     std::get<4>(it->second),
                                     std::get<5>(it->second)
                                     );
        }
    }

    return result;
}

std::vector< std::shared_ptr<BaseData> >
decaf::
ConstructData::split(
        const std::vector<std::vector<int> >& range)
{
    std::vector< std::shared_ptr<BaseData> > result;
    for(unsigned int i = 0; i < range.size(); i++)
        result.push_back(std::make_shared<ConstructData>());

    //Sanity check
    int totalItems = 0;
    for(unsigned int i = 0; i < range.size(); i++)
        totalItems+= range.at(i).size();
    if(totalItems != getNbItems()){
        std::cout<<"ERROR : The number of items in the ranges ("<<totalItems
                 <<") does not match the number of items of the object ("
                 <<getNbItems()<<")"<<std::endl;
        return result;
    }

    for(std::map<std::string, datafield>::iterator it = container_->begin();
        it != container_->end(); it++)
    {
        // Splitting the current field
        std::vector<std::shared_ptr<BaseConstructData> > splitFields;
        splitFields = std::get<3>(it->second)->split(range, std::get<4>(it->second));

        // Inserting the splitted field into the splitted results
        if(splitFields.size() != result.size())
        {
            std::cout<<"ERROR : A field was not splited properly."
                    <<" The number of chunks does not match the expected number of chunks"<<std::endl;
            // Cleaning the result to avoid corrupt data
            result.clear();

            return result;
        }

        //Adding the splitted results into the splitted maps
        for(unsigned int i = 0; i < result.size(); i++)
        {
            std::shared_ptr<ConstructData> construct = dynamic_pointer_cast<ConstructData>(result.at(i));
            construct->appendData(it->first,
                                     splitFields.at(i),
                                     std::get<0>(it->second),
                                     std::get<1>(it->second),
                                     std::get<4>(it->second),
                                     std::get<5>(it->second)
                                     );
        }
    }

    return result;
}

//Todo : remove the code redundancy
bool
decaf::
ConstructData::merge(shared_ptr<BaseData> other)
{
    std::shared_ptr<ConstructData> otherConstruct = std::dynamic_pointer_cast<ConstructData>(other);
    if(!otherConstruct)
    {
        std::cout<<"ERROR : Trying to merge two objects which have not the same dynamic type"<<std::endl;
        return false;
    }

    //No data yet, we simply copy the data from the other map
    if(!data_ || container_->empty())
    {
        //TODO : DANGEROUS should use a copy function
        container_ = otherConstruct->container_;
        data_ = static_pointer_cast<void>(container_);
        nbItems_ = otherConstruct->nbItems_;
        nbFields_ = otherConstruct->nbFields_;
        bZCurveKey_ = otherConstruct->bZCurveKey_;
        zCurveKey_ = otherConstruct->zCurveKey_;
        bZCurveIndex_ = otherConstruct->bZCurveIndex_;
        zCurveIndex_ = otherConstruct->zCurveIndex_;
    }
    else
    {
        //We check that we can merge all the fields before merging
        if(container_->size() != otherConstruct->getMap()->size())
        {
            std::cout<<"Error : the map don't have the same number of field. Merge aborted."<<std::endl;
            return false;
        }

        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            std::map<std::string, datafield>::iterator otherIt = otherConstruct->getMap()->find(it->first);
            if( otherIt == otherConstruct->getMap()->end())
            {
                std::cout<<"Error : The field \""<<it->first<<"\" is present in the"
                         <<"In the original map but not in the other one. Merge aborted."<<std::endl;
                return false;
            }
            if( !std::get<3>(otherIt->second)->canMerge(std::get<3>(it->second)) )
                return false;
        }

        //TODO : Add a checking on the merge policy and number of items in case of a merge

        //We have done all the checking, now we can merge securely
        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            std::map<std::string, datafield>::iterator otherIt = otherConstruct->getMap()->find(it->first);

            if(! std::get<3>(it->second)->merge(std::get<3>(otherIt->second), std::get<5>(otherIt->second)) )
            {
                std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has be corrupted."<<std::endl;
                return false;
            }
            std::get<2>(it->second) = std::get<3>(it->second)->getNbItems();
        }
    }

    return updateMetaData();
}

//Todo : remove the code redundancy
bool
decaf::
ConstructData::merge(char* buffer, int size)
{
    in_serial_buffer_ = std::string(buffer, size);
    boost::iostreams::basic_array_source<char> device(in_serial_buffer_.data(), in_serial_buffer_.size());
    boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
    boost::archive::binary_iarchive ia(sout);

    fflush(stdout);

    if(!data_ || container_->empty())
    {
        ia >> container_;

        data_ = std::static_pointer_cast<void>(container_);
    }
    else
    {
        std::shared_ptr<std::map<std::string, datafield> > other;
        ia >> other;

        //We check that we can merge all the fields before merging
        if(container_->size() != other->size())
        {
            std::cout<<"Error : the map don't have the same number of field. Merge aborted."<<std::endl;
            return false;
        }

        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            std::map<std::string, datafield>::iterator otherIt = other->find(it->first);
            if( otherIt == other->end())
            {
                std::cout<<"Error : The field \""<<it->first<<"\" is present in the"
                         <<"In the original map but not in the other one. Merge aborted."<<std::endl;
                return false;
            }
            if( !std::get<3>(otherIt->second)->canMerge(std::get<3>(it->second)))
                return false;
        }

        //TODO : Add a checking on the merge policy and number of items in case of a merge

        //We have done all the checking, now we can merge securely
        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            std::map<std::string, datafield>::iterator otherIt = other->find(it->first);

            if(! std::get<3>(it->second)->merge(std::get<3>(otherIt->second),
                                                std::get<5>(otherIt->second)) )
            {
                std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has be corrupted."<<std::endl;
                return false;
            }
            std::get<2>(it->second) = std::get<3>(it->second)->getNbItems();
        }
    }

    return updateMetaData();
}

bool
decaf::
ConstructData::merge()
{
    boost::iostreams::basic_array_source<char> device(in_serial_buffer_.data(), in_serial_buffer_.size());
    boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
    boost::archive::binary_iarchive ia(sout);

    if(!data_ || container_->empty())
    {
        ia >> container_;

        data_ = std::static_pointer_cast<void>(container_);
    }
    else
    {
        std::shared_ptr<std::map<std::string, datafield> > other;
        ia >> other;

        //We check that we can merge all the fields before merging
        if(container_->size() != other->size())
        {
            std::cout<<"Error : the map don't have the same number of field. Merge aborted."<<std::endl;
            return false;
        }

        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            std::map<std::string, datafield>::iterator otherIt = other->find(it->first);
            if( otherIt == other->end())
            {
                std::cout<<"Error : The field \""<<it->first<<"\" is present in the"
                         <<"In the original map but not in the other one. Merge aborted."<<std::endl;
                return false;
            }
            if( !std::get<3>(otherIt->second)->canMerge(std::get<3>(it->second)) )
                return false;
        }

        //TODO : Add a checking on the merge policy and number of items in case of a merge

        //We have done all the checking, now we can merge securely
        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            std::map<std::string, datafield>::iterator otherIt = other->find(it->first);

            if(! std::get<3>(it->second)->merge(std::get<3>(otherIt->second),
                                                std::get<5>(otherIt->second)) )
            {
                std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has be corrupted."<<std::endl;
                return false;
            }
            std::get<2>(it->second) = std::get<3>(it->second)->getNbItems();
        }
    }

    return updateMetaData();
}

bool
decaf::
ConstructData::serialize()
{
    boost::iostreams::back_insert_device<std::string> inserter(out_serial_buffer_);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
    boost::archive::binary_oarchive oa(s);

    oa << container_;
    s.flush();

    return true;
}

bool
decaf::
ConstructData::unserialize()
{
    return false;
}

//Prepare enough space in the serial buffer
void
decaf::
ConstructData::allocate_serial_buffer(int size)
{
    in_serial_buffer_.resize(size);
}

char*
decaf::
ConstructData::getOutSerialBuffer(int* size)
{
    *size = out_serial_buffer_.size(); //+1 for the \n caractere
    return &out_serial_buffer_[0]; //Dangerous if the string gets reallocated
}

char*
decaf::
ConstructData::getOutSerialBuffer()
{
    return &out_serial_buffer_[0];
}

int
decaf::
ConstructData::getOutSerialBufferSize(){ return out_serial_buffer_.size();}

char*
decaf::
ConstructData::getInSerialBuffer(int* size)
{
    *size = in_serial_buffer_.size(); //+1 for the \n caractere
    return &in_serial_buffer_[0]; //Dangerous if the string gets reallocated
}

char*
decaf::
ConstructData::getInSerialBuffer()
{
    return &in_serial_buffer_[0];
}

int
decaf::
ConstructData::getInSerialBufferSize(){ return in_serial_buffer_.size(); }

void
decaf::
ConstructData::purgeData()
{
    //To purge the data we just have to clean the map and reset the metadatas
    container_->clear();
    nbFields_ = 0;
    nbItems_ = 0;
    bZCurveKey_ = false;
    bZCurveIndex_ = false;
    zCurveKey_.reset();
    zCurveIndex_.reset();
    splitable_ = false;
}

bool
decaf::
ConstructData::setData(std::shared_ptr<void> data)
{
    std::shared_ptr<std::map<std::string, datafield> > container =
            static_pointer_cast<std::map<std::string, datafield> >(data);

    if(!container){
        std::cout<<"ERROR : can not cast the data into the proper type."<<std::endl;
        return false;
    }

    //Checking is the map is valid and updating the informations
    int nbItems = 0, nbFields = 0;
    bool bZCurveKey = false, bZCurveIndex = false;
    std::shared_ptr<BaseConstructData> zCurveKey, zCurveIndex;
    for(std::map<std::string, datafield>::iterator it = container->begin();
        it != container->end(); it++)
    {
        // Checking that we can insert this data and keep spliting the data after
        // If we already have fields with several items and we insert a new field
        // with another number of items, we can't split automatically
        if(nbItems > 1 && nbItems != std::get<2>(it->second))
        {
            std::cout<<"ERROR : can add new field with "<<std::get<2>(it->second)<<" items."
                    <<" The current map has "<<nbItems<<" items. The number of items "
                    <<"of the new filed should be 1 or "<<nbItems<<std::endl;
            return false;
        }
        else // We still update the number of items
            nbItems_ = std::get<2>(it->second);

        if(std::get<0>(it->second) == DECAF_ZCURVEKEY)
        {
            bZCurveKey = true;
            zCurveKey = std::get<3>(it->second);
        }

        if(std::get<0>(it->second) == DECAF_ZCURVEINDEX)
        {
            bZCurveIndex = true;
            zCurveIndex = std::get<3>(it->second);
        }

        //The field is already in the map, we don't have to test the insert
        nbFields++;
    }

    // We have checked all the fields without issue, we can use this map
    //container_ = container.get();
    container_ = container;
    nbItems_ = nbItems;
    nbFields_ = nbFields;
    bZCurveKey_ = bZCurveKey;
    bZCurveIndex_ = bZCurveIndex;
    zCurveKey_ = zCurveKey;
    zCurveIndex_ = zCurveIndex;

    return true;

}

std::shared_ptr<BaseConstructData>
decaf::
ConstructData::getData(std::string key)
{
    std::map<std::string, datafield>::iterator it;
    it = container_->find(key);
    if(it == container_->end())
    {
        std::cout<<"ERROR : key "<<key<<" not found."<<std::endl;
        return std::shared_ptr<BaseConstructData>();
    }
    else
        return std::get<3>(it->second);
}


bool
decaf::
ConstructData::updateMetaData()
{
    //Checking is the map is valid and updating the informations
    nbItems_ = 0;
    nbFields_ = 0;
    bZCurveKey_ = false;
    bZCurveIndex_ = false;

    for(std::map<std::string, datafield>::iterator it = container_->begin();
        it != container_->end(); it++)
    {
        // Checking that we can insert this data and keep spliting the data after
        // If we already have fields with several items and we insert a new field
        // with another number of items, we can't split automatically
        if(nbItems_ > 1 && nbItems_ != std::get<2>(it->second))
        {
            std::cout<<"ERROR : can add new field with "<<std::get<2>(it->second)<<" items."
                    <<" The current map has "<<nbItems_<<" items. The number of items "
                    <<"of the new filed should be 1 or "<<nbItems_<<std::endl;
            return false;
        }
        else // We still update the number of items
            nbItems_ = std::get<2>(it->second);

        if(std::get<0>(it->second) == DECAF_ZCURVEKEY)
        {
            bZCurveKey_ = true;
            zCurveKey_ = std::get<3>(it->second);
        }

        if(std::get<0>(it->second) == DECAF_ZCURVEINDEX)
        {
            bZCurveIndex_ = true;
            zCurveIndex_ = std::get<3>(it->second);
        }
        //The field is already in the map, we don't have to test the insert
        nbFields_++;
    }

    return true;
}

