#include <bredala/data_model/constructtype.h>
#include <bredala/data_model/arrayconstructdata.hpp>
#include <bredala/data_model/morton.h>
#include <sys/time.h>

#include <bredala/data_model/simpleconstructdata.hpp>

#include <bredala/data_model/maptools.h>
#include <bredala/data_model/pconstructtype.h>

using namespace decaf;
using namespace std;


// DEPRECATE, for experiments in Bredala paper only
double timeGlobalSplit = 0.0;
double timeGlobalBuild = 0.0;
double timeGlobalMerge = 0.0;
double timeGlobalRedist = 0.0;
double timeGlobalReceiv = 0.0;

bool
decaf::
ConstructData::setMergeOrder(std::vector<std::string>& merge_order)
{
	//TODO : check the name of the fields as well
	if(merge_order.size() == container_->size())
	{
		merge_order_ = merge_order;
		return true;
	}
	else
		return false;
}

const std::vector<std::string>&
decaf::
ConstructData::getMergeOrder()
{
	return merge_order_;
}

bool
decaf::
ConstructData::setSplitOrder(std::vector<std::string>& split_order)
{
	//TODO : check the name of the fields as well
	if(split_order.size() == container_->size())
	{
		split_order_ = split_order;
		return true;
	}
	else
		return false;
}

const std::vector<string> &decaf::ConstructData::getSplitOrder()
{
	return split_order_;
}


decaf::
ConstructData::ConstructData() :
    BaseData(), nbFields_(0), bZCurveIndex_(false),
    zCurveIndex_(NULL), bZCurveKey_(false), zCurveKey_(NULL),
    bSystem_(false), nbSystemFields_(0), bEmpty_(true),
    bToken_(false),bCountable_(true), bPartialCountable_(true)
{
	container_ = std::make_shared<std::map<std::string, datafield> >();
	//data_ = static_pointer_cast<void>(container_);
}

bool
decaf::
ConstructData::appendItem(std::shared_ptr<ConstructData> dest, unsigned int index)
{
	assert(bCountable_);

	if(index >= nbItems_)
	{
		std::cout<<"ERROR : Trying to extract an item out of range (requesting "<<index<<", "<<nbItems_<<" in the container)."<<std::endl;
		return false;
	}

	bool result;
	for(std::map<std::string, datafield>::iterator it = container_->begin(); it != container_->end(); it++)
	{
		std::map<std::string, datafield>::iterator itDest = dest->container_->find(it->first);
		if(itDest == dest->container_->end())
		{
			std::cout<<"ERROR : The field "<<it->first<<" is not available in the destination container."<<std::endl;
			return false;
		}
		std::shared_ptr<BaseConstructData> fieldDest = getBaseData(itDest->second);
		result = getBaseData(it->second)->appendItem(fieldDest, index, getMergePolicy(it->second));

		if(!result)
			return false;

		getNbItemsField(itDest->second) = getNbItemsField(itDest->second) + 1;
	}

	return true;
}

bool
decaf::
ConstructData::appendData(string name,
                          std::shared_ptr<BaseConstructData>  data,
                          ConstructTypeFlag flags,
                          ConstructTypeScope scope,
                          ConstructTypeSplitPolicy splitFlag,
                          ConstructTypeMergePolicy mergeFlag)
{
	std::pair<std::map<std::string, datafield>::iterator,bool> ret;
	datafield newEntry = make_tuple(flags, scope, data->getNbItems(), data, splitFlag, mergeFlag);
	ret = container_->insert(std::pair<std::string, datafield>(name, newEntry));

	if(ret.second && (!merge_order_.empty() || !split_order_.empty()))
	{
		std::cout<<"New field added. The priority split/merge list is invalid. Clearing."<<std::endl;
		merge_order_.clear();
		split_order_.clear();
	}

	if(!ret.second)
	{
		fprintf(stderr, "ERROR : appendData failed. A field named \"%s\" already exist in the data model.\n", name.c_str() );
		return false;
	}

	return updateMetaData();
}

bool
decaf::
ConstructData::appendData(std::string name,
                          BaseField&  data,
                          ConstructTypeFlag flags,
                          ConstructTypeScope scope,
                          ConstructTypeSplitPolicy splitFlag,
                          ConstructTypeMergePolicy mergeFlag)
{
	std::pair<std::map<std::string, datafield>::iterator,bool> ret;
	datafield newEntry = make_tuple(flags, scope, data->getNbItems(), data.getBasePtr(), splitFlag, mergeFlag);
	ret = container_->insert(std::pair<std::string, datafield>(name, newEntry));

	if(ret.second && (!merge_order_.empty() || !split_order_.empty()))
	{
		std::cout<<"New field added. The priority split/merge list is invalid. Clearing."<<std::endl;
		merge_order_.clear();
		split_order_.clear();
	}

	if(!ret.second)
	{
		fprintf(stderr, "ERROR : appendData failed. A field named \"%s\" already exist in the data model.\n", name.c_str() );
		return false;
	}

	return updateMetaData();
}

bool
decaf::
ConstructData::appendData(const char* name,
                          std::shared_ptr<BaseConstructData>  data,
                          ConstructTypeFlag flags,
                          ConstructTypeScope scope,
                          ConstructTypeSplitPolicy splitFlag,
                          ConstructTypeMergePolicy mergeFlag)
{
	std::pair<std::map<std::string, datafield>::iterator,bool> ret;
	datafield newEntry = make_tuple(flags, scope, data->getNbItems(), data, splitFlag, mergeFlag);
	ret = container_->insert(std::pair<std::string, datafield>(name, newEntry));

	if(ret.second && (!merge_order_.empty() || !split_order_.empty()))
	{
		std::cout<<"New field added. The priority split/merge list is invalid. Clearing."<<std::endl;
		merge_order_.clear();
		split_order_.clear();
	}

	if(!ret.second)
	{
		fprintf(stderr, "ERROR : appendData failed. A field named \"%s\" already exist in the data model.\n", name );
		return false;
	}

	return updateMetaData();
}

bool
decaf::
ConstructData::appendData(const char* name,
                          BaseField&  data,
                          ConstructTypeFlag flags,
                          ConstructTypeScope scope,
                          ConstructTypeSplitPolicy splitFlag,
                          ConstructTypeMergePolicy mergeFlag)
{
	std::pair<std::map<std::string, datafield>::iterator,bool> ret;
	datafield newEntry = make_tuple(flags, scope, data->getNbItems(), data.getBasePtr(), splitFlag, mergeFlag);
	ret = container_->insert(std::pair<std::string, datafield>(name, newEntry));

	if(ret.second && (!merge_order_.empty() || !split_order_.empty()))
	{
		std::cout<<"New field added. The priority split/merge list is invalid. Clearing."<<std::endl;
		merge_order_.clear();
		split_order_.clear();
	}

	if(!ret.second)
	{
		fprintf(stderr, "ERROR : appendData failed. A field named \"%s\" already exist in the data model.\n", name );
		return false;
	}

	return updateMetaData();
}



bool
decaf::
ConstructData::appendData(pConstructData data, const string name)
{
	// This is only safe if the field is present in the data model
	std::pair<std::map<std::string, datafield>::iterator,bool> ret;
	ret = container_->insert(std::pair<std::string, datafield>(name, data->container_->at(name)));

	if(ret.second && (!merge_order_.empty() || !split_order_.empty()))
	{
		std::cout<<"New field added. The priority split/merge list is invalid. Clearing."<<std::endl;
		merge_order_.clear();
		split_order_.clear();
	}

	if(!ret.second)
	{
		fprintf(stderr, "ERROR : appendData failed. A field named \"%s\" already exist in the data model.\n", name.c_str() );
		return false;
	}

	return updateMetaData();
}


bool
decaf::
ConstructData::appendData(pConstructData data, const char* name)
{
	// This is only safe if the field is present in the data model
	std::pair<std::map<std::string, datafield>::iterator,bool> ret;
	ret = container_->insert(std::pair<std::string, datafield>(name, data->container_->at(name)));

	if(ret.second && (!merge_order_.empty() || !split_order_.empty()))
	{
		std::cout<<"New field added. The priority split/merge list is invalid. Clearing."<<std::endl;
		merge_order_.clear();
		split_order_.clear();
	}

	if(!ret.second)
	{
		fprintf(stderr, "ERROR : appendData failed. A field named \"%s\" already exist in the data model.\n", name );
		return false;
	}

	return updateMetaData();
}




bool
decaf::
ConstructData::removeData(std::string name)
{
	std::map<std::string, datafield>::iterator it = container_->find(name);
	if(it != container_->end())
	{
		container_->erase(it);
		if(!merge_order_.empty() || !split_order_.empty())
		{
			std::cout<<"Field erased. The priority split/merge list is invalid. Clearing."<<std::endl;
			merge_order_.clear();
			split_order_.clear();
		}

		return updateMetaData();
	}

	return false;
}

bool
decaf::
ConstructData::isCountable()
{
	return bCountable_;
}

bool
decaf::
ConstructData::isPartiallyCountable()
{
	return bPartialCountable_;
}

bool
decaf::
ConstructData::updateData(std::string name,
                          std::shared_ptr<BaseConstructData>  data)
{
	std::map<std::string, datafield>::iterator it = container_->find(name);
	if(it != container_->end())
	{
		std::get<3>(it->second) = data;
		return updateMetaData();
	}
	else
	{
		std::cerr<<"ERROR : field \'"<<name<<"\' not found in the map. "
		        <<"Unable to update the map."<<std::endl;
		return false;
	}

}

int
decaf::
ConstructData::getNbFields()
{
	return nbFields_;
}

bool
decaf::
ConstructData::getTypename(std::string &name, std::string &type){
	std::shared_ptr<BaseConstructData> field = this->getData(name);
	if(!field){
		return false;
	}
	type = field->getTypename();
	return true;
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
	fprintf(stderr, "Current state of the map: \n");
	for(std::map<std::string, datafield>::iterator it = container_->begin();
	            it != container_->end(); it++)
		fprintf(stderr, "Key: %s; nbItems: %d\n", it->first.c_str(), getNbItemsField(it->second));
	fprintf(stderr, "End of display of the map\n");

}

vector<std::string>
decaf::
ConstructData::listUserKeys(){
	vector<string> list;
	for(std::map<std::string, datafield>::iterator it = container_->begin();
	            it != container_->end(); it++){
		if( getScope(it->second) != DECAF_SYSTEM ){
			list.push_back(it->first);
		}
	}
	return list;
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
ConstructData::isSystem()
{
	return bSystem_;
}

void
decaf::
ConstructData::setSystem(bool bSystem)
{
	bSystem_ = bSystem;
}

bool
decaf::
ConstructData::hasSystem()
{
	return nbSystemFields_ > 0;
}

bool
decaf::
ConstructData::isEmpty()
{
	return bEmpty_;
}

void
decaf::
ConstructData::setToken(bool bToken)
{
    bToken_ = bToken;
}

bool
decaf::
ConstructData::isToken()
{
    return bToken_;
}


bool
decaf::
ConstructData::isSplitable()
{
	return bCountable_ && nbItems_ > 0;
}

std::vector< std::shared_ptr<BaseData> >
decaf::
ConstructData::split(const std::vector<int>& range)
{
	std::vector< std::shared_ptr<BaseData> > result;
	std::vector< mapConstruct > result_maps;
	for(unsigned int i = 0; i < range.size(); i++)
	{
		result.push_back(std::make_shared<ConstructData>());
		std::shared_ptr<ConstructData> construct =
		        dynamic_pointer_cast<ConstructData>(result.back());
		result_maps.push_back(construct->getMap());
	}

	if(!bCountable_)
	{
		fprintf(stderr,"ERROR : trying to split non countable data model.\n");
		return result;
	}

	//Sanity check
	int totalRange = 0;
	for(unsigned int i = 0; i < range.size(); i++)
		totalRange+= range.at(i);
	if(totalRange != getNbItems()){
		fprintf(stderr, "ERROR : The number of items in the ranges (%d) does not match the "
		                "number of items of the object (%d)\n", totalRange, getNbItems());
		return result;
	}

	//Splitting data
	if(split_order_.size() > 0) //Splitting from the user order
	{
		for(unsigned int i = 0; i < split_order_.size(); i++)
		{
			std::map<std::string, datafield>::iterator data  = container_->find(split_order_.at(i));
			if(data == container_->end())
			{
				std::cerr<<"ERROR : field \""<<split_order_.at(i)<<"\" provided by the user to "
				        <<"split the data not found in the map."<<std::endl;
				return result;

			}
			// Splitting the current field
			std::vector<std::shared_ptr<BaseConstructData> > splitFields;
			splitFields = getBaseData(data->second)->split(range, result_maps, getSplitPolicy(data->second));

			// Inserting the splitted field into the splitted results
			if(splitFields.size() != result.size())
			{
				fprintf(stderr, "ERROR : A field was not split properly."
				                " The number of chunks does not match the expected number of chunks\n");

				// Cleaning the result to avoid corrupt data
				result.clear();

				return result;
			}

			//Adding the splitted results into the splitted maps
			for(unsigned int j = 0; j < result.size(); j++)
			{
				std::shared_ptr<ConstructData> construct = dynamic_pointer_cast<ConstructData>(result.at(j));
				construct->appendData(split_order_.at(i),
				                      splitFields.at(j),
				                      getFlag(data->second),
				                      std::get<1>(data->second),
				                      getSplitPolicy(data->second),
				                      getMergePolicy(data->second)
				                      );
			}
		}
	}
	else
	{
		for(std::map<std::string, datafield>::iterator it = container_->begin();
		    it != container_->end(); it++)
		{
			// Splitting the current field
			std::vector<std::shared_ptr<BaseConstructData> > splitFields;
			splitFields = getBaseData(it->second)->split(range, result_maps, getSplitPolicy(it->second));

			// Inserting the splitted field into the splitted results
			if(splitFields.size() != result.size())
			{
				fprintf(stderr, "ERROR : A field was not split properly."
				                " The number of chunks does not match the expected number of chunks\n");

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
				                      getFlag(it->second),
				                      std::get<1>(it->second),
				                      getSplitPolicy(it->second),
				                      getMergePolicy(it->second)
				                      );
			}
		}
	}

	return result;
}

void
decaf::
ConstructData::split(const std::vector<int>& range,
                     std::vector<pConstructData> buffers)
{
	std::vector< mapConstruct > result_maps;
	for(unsigned int i = 0; i < range.size(); i++)
	{
		result_maps.push_back(buffers[i]->getMap());
	}

	if(!bCountable_)
	{
		fprintf(stderr,"ERROR : trying to split non countable data model.\n");
		return;
	}

	//Sanity check
	int totalRange = 0;
	for(unsigned int i = 0; i < range.size(); i++)
		totalRange+= range.at(i);
	if(totalRange > getNbItems()){
		fprintf(stderr, "ERROR : The number of items in the ranges (%d) does not match the "
		                "number of items of the object (%d)\n", totalRange, getNbItems());
		return ;
	}

	//Splitting data
	if(split_order_.size() > 0) //Splitting from the user order
	{
		for(unsigned int i = 0; i < split_order_.size(); i++)
		{
			std::map<std::string, datafield>::iterator data  = container_->find(split_order_.at(i));
			if(data == container_->end())
			{
				std::cerr<<"ERROR : field \""<<split_order_.at(i)<<"\" provided by the user to "
				        <<"split the data not found in the map."<<std::endl;
				return ;

			}

			//Building a temp vector with all the fields
			std::vector<std::shared_ptr<BaseConstructData> > fields;
			for(unsigned int i = 0; i < buffers.size(); i++)
			{
				std::shared_ptr<BaseConstructData> field = buffers[i]->getData(split_order_.at(i));
				if(field)
					fields.push_back(field);
				else
				{
					std::cout<<"ERROR : the field "<<split_order_.at(i)<<" is not present in the preallocated container."<<std::endl;
					return;
				}
			}

			getBaseData(data->second)->split(range, result_maps, fields, getSplitPolicy(data->second));
		}
	}
	else
	{
		for(std::map<std::string, datafield>::iterator it = container_->begin();
		    it != container_->end(); it++)
		{
			//Building a temp vector with all the fields
			std::vector<std::shared_ptr<BaseConstructData> > fields;
			for(unsigned int i = 0; i < buffers.size(); i++)
			{
				std::shared_ptr<BaseConstructData> field = buffers[i]->getData(it->first);
				if(field)
					fields.push_back(field);
				else
				{
					std::cout<<"ERROR : the field "<<it->first<<" is not present in the preallocated container."<<std::endl;
					//result.clear();
					//return result;
					return;
				}
			}

			// Splitting the current field
			getBaseData(it->second)->split(range, result_maps, fields, getSplitPolicy(it->second));
		}
	}

	for(unsigned int i = 0; i < range.size(); i++)
	{
		buffers[i]->updateNbItems();
		buffers[i]->updateMetaData();
	}

	return;
}

std::vector< std::shared_ptr<BaseData> >
decaf::
ConstructData::split(
        const std::vector<std::vector<int> >& range)
{
	std::vector< std::shared_ptr<BaseData> > result;
	std::vector< mapConstruct > result_maps;
	for(unsigned int i = 0; i < range.size(); i++)
	{
		result.push_back(std::make_shared<ConstructData>());
		std::shared_ptr<ConstructData> construct =
		        dynamic_pointer_cast<ConstructData>(result.back());
		result_maps.push_back(construct->getMap());
	}

	if(!bCountable_)
	{
		fprintf(stderr,"ERROR : trying to split non countable data model.\n");
		return result;
	}

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

	//Splitting data
	if(split_order_.size() > 0) //Splitting from the user order
	{
		for(unsigned int i = 0; i < split_order_.size(); i++)
		{
			std::map<std::string, datafield>::iterator data  = container_->find(split_order_.at(i));
			if(data == container_->end())
			{
				std::cerr<<"ERROR : field \""<<split_order_.at(i)<<"\" provided by the user to "
				        <<"split the data not found in the map."<<std::endl;
				return result;

			}
			// Splitting the current field
			std::vector<std::shared_ptr<BaseConstructData> > splitFields;
			splitFields = getBaseData(data->second)->split(range, result_maps, getSplitPolicy(data->second));

			// Inserting the splitted field into the splitted results
			if(splitFields.size() != result.size())
			{
				fprintf(stderr, "ERROR : A field was not split properly."
				                " The number of chunks does not match the expected number of chunks\n");

				// Cleaning the result to avoid corrupt data
				result.clear();

				return result;
			}

			//Adding the splitted results into the splitted maps
			for(unsigned int j = 0; j < result.size(); j++)
			{
				std::shared_ptr<ConstructData> construct = dynamic_pointer_cast<ConstructData>(result.at(j));
				construct->appendData(split_order_.at(i),
				                      splitFields.at(j),
				                      getFlag(data->second),
				                      std::get<1>(data->second),
				                      getSplitPolicy(data->second),
				                      getMergePolicy(data->second)
				                      );
			}
		}
	}
	else    //Splitting from the map order
	{
		for(std::map<std::string, datafield>::iterator it = container_->begin();
		    it != container_->end(); it++)
		{
			// Splitting the current field
			std::vector<std::shared_ptr<BaseConstructData> > splitFields;
			splitFields = getBaseData(it->second)->split(range, result_maps, getSplitPolicy(it->second));

			// Inserting the splitted field into the splitted results
			if(splitFields.size() != result.size())
			{
				fprintf(stderr, "ERROR : A field was not split properly."
				                " The number of chunks does not match the expected number of chunks\n");

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
				                      getFlag(it->second),
				                      std::get<1>(it->second),
				                      getSplitPolicy(it->second),
				                      getMergePolicy(it->second)
				                      );
			}
		}
	}

	return result;
}

void
decaf::
ConstructData::split(const std::vector<std::vector<int> >& range,
                     std::vector<pConstructData> buffers)
{
	std::vector< mapConstruct > result_maps;
	for(unsigned int i = 0; i < range.size(); i++)
	{
		result_maps.push_back(buffers[i]->getMap());
	}

	if(!bCountable_)
	{
		fprintf(stderr,"ERROR : trying to split non countable data model.\n");
		return;
	}

	// Sanity check
	// REMOVED : some items may be duplicated for ghost regions for instance
	/*int totalItems = 0;
	for(unsigned int i = 0; i < range.size(); i++)
		totalItems+= range.at(i).back();

	if(totalItems > getNbItems()){
		std::cout<<"ERROR : The number of items in the ranges ("<<totalItems
				<<") does not match the number of items of the object ("
			   <<getNbItems()<<")"<<std::endl;
		return ;
	}*/

	//std::cout<<"Total number of item to split : "<<totalItems<<std::endl;

	//Splitting data
	if(split_order_.size() > 0) //Splitting from the user order
	{
		for(unsigned int i = 0; i < split_order_.size(); i++)
		{
			std::map<std::string, datafield>::iterator data  = container_->find(split_order_.at(i));
			if(data == container_->end())
			{
				std::cerr<<"ERROR : field \""<<split_order_.at(i)<<"\" provided by the user to "
				        <<"split the data not found in the map."<<std::endl;
				return;

			}

			//Building a temp vector with all the fields
			std::vector<std::shared_ptr<BaseConstructData> > fields;
			for(unsigned int i = 0; i < buffers.size(); i++)
			{
				std::shared_ptr<BaseConstructData> field = buffers[i]->getData(split_order_.at(i));
				if(field)
					fields.push_back(field);
				else
				{
					std::cout<<"ERROR : the field "<<split_order_.at(i)<<" is not present in the preallocated container."<<std::endl;
					return;
				}
			}

			getBaseData(data->second)->split(range, result_maps, fields, getSplitPolicy(data->second));
		}
	}
	else    //Splitting from the map order
	{
		for(std::map<std::string, datafield>::iterator it = container_->begin();
		    it != container_->end(); it++)
		{
			// Splitting the current field
			//Building a temp vector with all the fields
			std::vector<std::shared_ptr<BaseConstructData> > fields;
			for(unsigned int i = 0; i < buffers.size(); i++)
			{
				std::shared_ptr<BaseConstructData> field = buffers[i]->getData(it->first);
				if(field)
					fields.push_back(field);
				else
				{
					std::cout<<"ERROR : the field "<<it->first<<" is not present in the preallocated container."<<std::endl;
					return;
				}
			}
			getBaseData(it->second)->split(range, result_maps, fields, getSplitPolicy(it->second));

		}
	}

	for(unsigned int i = 0; i < range.size(); i++)
	{
		buffers[i]->updateNbItems();
		buffers[i]->updateMetaData();
	}

	return ;
}


void
computeIndexesFromBlocks(
        const std::vector<Block<3> >& blocks,
        float* pos,
        unsigned int nbPos,
        std::vector<std::vector<int> > &result)
{
	assert(result.size() == blocks.size());
	struct timeval begin;
	struct timeval end;

	gettimeofday(&begin, NULL);
	int notInBlock = 0;

	std::vector<unsigned int> sumPos(result.size(), 0);
	for(int i = 0; i < nbPos; i++)
	{
		bool particlesInBlock = false;
		for(unsigned int b = 0; b < blocks.size(); b++)
		{
			float x = pos[3*i];
			float y = pos[3*i+1];
			float z = pos[3*i+2];

			if(blocks[b].isInLocalBlock(x, y, z))
			{

				//                result[b].push_back(i);
				//                particlesInBlock = true;
				// Case for the first element
				if(result[b].empty())
				{
					result[b].push_back(i);
					result[b].push_back(1);
				}
				// Case where the current position is following the previous one
				else if(result[b].back() + (result[b])[result[b].size()-2] == i )
				{
					result[b].back() = result[b].back() + 1;
				}
				else
				{
					result[b].push_back(i);
					result[b].push_back(1);
				}

				sumPos[b] = sumPos[b] + 1;
				particlesInBlock = true;
			}
		}
		if(!particlesInBlock)
		{
			std::cout<<"Not attributed : ["<<pos[3*i]<<","<<pos[3*i+1]<<","<<pos[3*i+2]<<"]"<<std::endl;
			notInBlock++;
		}
	}

	//Pushing the total at the end of the vector
	int total = 0;
	for(unsigned int i = 0; i < blocks.size(); i++)
	{
		result[i].push_back(sumPos[i]);
		total += sumPos[i];
	}
	//printf("Somme des particules reparties : %u\n", total);

	gettimeofday(&end, NULL);
	double computeSplit = end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);

	//printf("Computation of the particule attribution (morton): %f\n", computeSplit);
	//timeGlobalMorton += computeSplit;
	return;
}

void
computeIndexesFromBlocks(
        const std::vector<Block<3> > &blocks,
        unsigned int *pos,
        unsigned int nbPos,
        std::vector<std::vector<int> > &result)
{
	assert(result.size() == blocks.size());
	struct timeval begin;
	struct timeval end;

	gettimeofday(&begin, NULL);
	int notInBlock = 0;

	std::vector<unsigned int> sumPos(result.size(), 0);
	for(int i = 0; i < nbPos; i++)
	{
		unsigned int x,y,z;
		Morton_3D_Decode_10bit(pos[i], x, y, z);
		bool particlesInBlock = false;
		for(unsigned int b = 0; b < blocks.size(); b++)
		{
			if(blocks[b].isInLocalBlock(x, y, z))
			{
				//                result[b].push_back(i);
				//                particlesInBlock = true;
				// Case for the first element
				if(result[b].empty())
				{
					result[b].push_back(i);
					result[b].push_back(1);
				}
				// Case where the current position is following the previous one
				else if(result[b].back() + (result[b])[result[b].size()-2] == i )
				{
					result[b].back() = result[b].back() + 1;
				}
				else
				{
					result[b].push_back(i);
					result[b].push_back(1);
				}

				sumPos[b] = sumPos[b] + 1;
				particlesInBlock = true;
			}
		}
		if(!particlesInBlock)
		{
			std::cout<<"Not attributed : ["<<pos[i]<<"]"<<std::endl;
			notInBlock++;
		}
	}

	//Pushing the total at the end of the vector
	int total = 0;
	for(unsigned int i = 0; i < blocks.size(); i++)
	{
		result[i].push_back(sumPos[i]);
		total += sumPos[i];
	}
	//printf("Somme des particules reparties : %u\n", total);

	gettimeofday(&end, NULL);
	double computeSplit = end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);

	//printf("Computation of the particule attribution (morton): %f\n", computeSplit);
	//timeGlobalMorton += computeSplit;
	return;
}

/*std::vector< std::shared_ptr<BaseData> >
decaf::
ConstructData::split(const std::vector<Block<3> >& range)
{
	struct timeval begin;
	struct timeval end;

	gettimeofday(&begin, NULL);
	std::vector< std::shared_ptr<ConstructData> > container;
	std::vector< std::shared_ptr<BaseData> >result;

	preallocMultiple( range.size() , 30000, container);
	for(unsigned int i = 0; i < container.size(); i++)
		result.push_back(container.at(i));

	if(bZCurveIndex_)
	{
		std::shared_ptr<ArrayConstructData<unsigned int> > posData =
			std::dynamic_pointer_cast<ArrayConstructData<unsigned int> >(zCurveIndex_);

	int nbMortons = posData->getNbItems();
	unsigned int* morton = posData->getArray();
		//Going through all the morton codes and pushing the items
	for(int i = 0; i < nbMortons; i++)
		{
			unsigned int x,y,z;
			Morton_3D_Decode_10bit(morton[i], x, y, z);
			bool particlesInBlock = false;
			for(unsigned int b = 0; b < range.size(); b++)
			{
				if(range[b].isInLocalBlock(x, y, z))
				{
			this->appendItem(container[b],i);
		}
		}
	}
	}
	else
	{
		std::cout<<"ERROR : split by block not implemented when the morton code are not in the data model."<<std::endl;
	}

	for(unsigned int i = 0; i < container.size(); i++)
	container.at(i)->updateMetaData();
	return result;
}
*/

std::vector< std::shared_ptr<BaseData> >
decaf::
ConstructData::split(const std::vector<Block<3> >& range)
{

	struct timeval begin;
	struct timeval end;

	gettimeofday(&begin, NULL);
	std::vector< std::shared_ptr<BaseData> > result;
	std::vector< mapConstruct > result_maps;
	for(unsigned int i = 0; i < range.size(); i++)
	{
		result.push_back(std::make_shared<ConstructData>());
		std::shared_ptr<ConstructData> construct =
		        dynamic_pointer_cast<ConstructData>(result.back());
		result_maps.push_back(construct->getMap());
	}

	//Preparing the ranges if some fields are not splitable with a block
	bool computeRanges = false;
	std::vector<std::vector<int> > rangeItems;
	for(unsigned int i = 0; i < range.size(); i++)
		rangeItems.push_back(std::vector<int>());

	//Splitting data
	if(split_order_.size() > 0) //Splitting from the user order
	{
		for(unsigned int i = 0; i < split_order_.size(); i++)
		{
			std::map<std::string, datafield>::iterator data  = container_->find(split_order_.at(i));
			if(data == container_->end())
			{
				std::cerr<<"ERROR : field \""<<split_order_.at(i)<<"\" provided by the user to "
				        <<"split the data not found in the map."<<std::endl;
				return result;

			}
			// Splitting the current field
			std::vector<std::shared_ptr<BaseConstructData> > splitFields;
			if(getBaseData(data->second)->isBlockSplitable())
				splitFields = getBaseData(data->second)->split(range, result_maps, getSplitPolicy(data->second));
			else
			{
				// Checking if the field is countable
				if(!bPartialCountable_ || !getBaseData(data->second)->isCountable())
				{
					std::cerr<<"ERROR : The field \""<<data->first<<"\" is not splitable by "
					        <<"block and is not countable. Aborting the split."<<std::endl;
					return result;
				}

				if(!computeRanges)
				{
					//The ranges by items have not been computed yet
					//We need the Morton key to compute them
					if(bZCurveIndex_)
					{
						std::shared_ptr<ArrayConstructData<unsigned int> > posData =
						        std::dynamic_pointer_cast<ArrayConstructData<unsigned int> >(zCurveIndex_);
						computeIndexesFromBlocks(range, posData->getArray(), posData->getNbItems(), rangeItems);
						computeRanges = true;
					}
					else if(bZCurveKey_)
					{
						std::shared_ptr<ArrayConstructData<float> > posData =
						        std::dynamic_pointer_cast<ArrayConstructData<float> >(zCurveKey_);
						computeIndexesFromBlocks(range, posData->getArray(), posData->getNbItems(), rangeItems);
						computeRanges = true;
					}
					else
					{
						std::cerr<<"ERROR : The field \""<<split_order_.at(i)<<"\" is not splitable by "
						        <<"block and the container doesn't have a field with the flag ZCURVE_KEY."<<std::endl;

						return result;
					}

				}

				splitFields = getBaseData(data->second)->split(rangeItems, result_maps, getSplitPolicy(data->second));
			}

			// Inserting the splitted field into the splitted results
			if(splitFields.size() != result.size())
			{
				fprintf(stderr, "ERROR : A field was not split properly."
				                " The number of chunks does not match the expected number of chunks\n");

				// Cleaning the result to avoid corrupt data
				result.clear();

				return result;
			}

			//Adding the splitted results into the splitted maps
			for(unsigned int j = 0; j < result.size(); j++)
			{
				std::shared_ptr<ConstructData> construct = dynamic_pointer_cast<ConstructData>(result.at(j));
				construct->appendData(split_order_.at(i),
				                      splitFields.at(j),
				                      getFlag(data->second),
				                      std::get<1>(data->second),
				                      getSplitPolicy(data->second),
				                      getMergePolicy(data->second)
				                      );
			}
		}
	}
	else
	{
		for(std::map<std::string, datafield>::iterator it = container_->begin();
		    it != container_->end(); it++)
		{
			// Splitting the current field
			std::vector<std::shared_ptr<BaseConstructData> > splitFields;
			if(getBaseData(it->second)->isBlockSplitable())
				splitFields = getBaseData(it->second)->split(range, result_maps, getSplitPolicy(it->second));
			else
			{
				// Checking if the field is countable
				if(!bPartialCountable_ || !getBaseData(it->second)->isCountable())
				{
					std::cerr<<"ERROR : The field \""<<it->first<<"\" is not splitable by "
					        <<"block and is not countable. Aborting the split."<<std::endl;
					return result;
				}

				if(!computeRanges)
				{
					//The ranges by items have not been computed yet
					//We need the Morton key to compute them
					if(bZCurveIndex_)
					{
						std::shared_ptr<ArrayConstructData<unsigned int> > posData =
						        std::dynamic_pointer_cast<ArrayConstructData<unsigned int> >(zCurveIndex_);
						computeIndexesFromBlocks(range, posData->getArray(), posData->getNbItems(), rangeItems);
						computeRanges = true;
					}
					else if(bZCurveKey_)
					{
						std::shared_ptr<ArrayConstructData<float> > posData =
						        std::dynamic_pointer_cast<ArrayConstructData<float> >(zCurveKey_);
						computeIndexesFromBlocks(range, posData->getArray(), posData->getNbItems(), rangeItems);
						computeRanges = true;
					}
					else
					{
						std::cerr<<"ERROR : The field \""<<it->first<<"\" is not splitable by "
						        <<"block and the container doesn't have a field with the flag ZCURVE_KEY."<<std::endl;

						return result;
					}
				}

				splitFields = getBaseData(it->second)->split(rangeItems, result_maps, getSplitPolicy(it->second));
			}

			// Inserting the splitted field into the splitted results
			if(splitFields.size() != result.size())
			{
				std::cout<<"ERROR : The field "<<it->first<<" was not splited properly."
				        <<" The number of chunks does not match the expected number of chunks"<<std::endl;
				// Cleaning the result to avoid corrupt data
				result.clear();

				return result;
			}

			//Adding the splitted results into the splitted maps
			for(unsigned int j = 0; j < result.size(); j++)
			{
				std::shared_ptr<ConstructData> construct = dynamic_pointer_cast<ConstructData>(result.at(j));
				construct->appendData(it->first,
				                      splitFields.at(j),
				                      getFlag(it->second),
				                      std::get<1>(it->second),
				                      getSplitPolicy(it->second),
				                      getMergePolicy(it->second)
				                      );
			}
		}
	}

	assert(result.size() == range.size());
	gettimeofday(&end, NULL);
	//timeGlobalSplit += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
	return result;
}

void
decaf::
ConstructData::split(
        const std::vector<Block<3> >& range,
        std::vector<pConstructData >& buffers)
{
	struct timeval begin;
	struct timeval end;

	gettimeofday(&begin, NULL);
	std::vector< mapConstruct > result_maps;
	for(unsigned int i = 0; i < range.size(); i++)
	{
		result_maps.push_back(buffers.at(i)->getMap());
	}

	//Preparing the ranges if some fields are not splitable with a block
	bool computeRanges = false;
	if(rangeItems_.empty())
	{
		for(unsigned int i = 0; i < range.size(); i++)
		{
			rangeItems_.push_back(std::vector<int>());
			rangeItems_[i].reserve(873);	//Valeur pour les benchs Article Cluster. A retirer
		}
	}
	else
	{
		for(unsigned int i = 0; i < range.size(); i++)
		{
			std::cout<<"Capacity before clear : "<<rangeItems_[i].capacity()<<std::endl;
			rangeItems_[i].clear();
			std::cout<<"Capacity after clear : "<<rangeItems_[i].capacity()<<std::endl;
		}
	}


	for(std::map<std::string, datafield>::iterator it = container_->begin();
	    it != container_->end(); it++)
	{
		//Building a temp vector with all the fields
        std::vector<std::shared_ptr<BaseConstructData> > fields;
		for(unsigned int i = 0; i < buffers.size(); i++)
		{
			std::shared_ptr<BaseConstructData> field = buffers[i]->getData(it->first);
			if(field)
				fields.push_back(field);
			else
			{
                std::cout<<"ERROR : the field "<<it->first<<" is not present in the preallocated container."<<std::endl;
				//result.clear();
				//return result;
				return;
			}
		}
		// Splitting the current field
		// WARNING : should not be called, just for test
		std::vector<std::shared_ptr<BaseConstructData> > splitFields;
		if(getBaseData(it->second)->isBlockSplitable())
			splitFields = getBaseData(it->second)->split(range, result_maps, getSplitPolicy(it->second));
		//getBaseData(it->second)->split(range, result_maps, fields, getSplitPolicy(it->second));
		else
		{
			// Checking if the field is countable
			if(!bPartialCountable_ || !getBaseData(it->second)->isCountable())
			{
				std::cerr<<"ERROR : The field \""<<it->first<<"\" is not splitable by "
				        <<"block and is not countable. Aborting the split."<<std::endl;
				return;
			}

			// Checking if we have already converted the Block into a list of items
			if(!computeRanges)
			{
				//The ranges by items have not been computed yet
				//We need the Morton key to compute them
				if(bZCurveIndex_)
				{
					std::shared_ptr<ArrayConstructData<unsigned int> > posData =
					        std::dynamic_pointer_cast<ArrayConstructData<unsigned int> >(zCurveIndex_);
					computeIndexesFromBlocks(range, posData->getArray(), posData->getNbItems(), rangeItems_);
					computeRanges = true;
				}
				else if(bZCurveKey_)
				{
					std::shared_ptr<ArrayConstructData<float> > posData =
					        std::dynamic_pointer_cast<ArrayConstructData<float> >(zCurveKey_);
					if(!posData) std::cout<<"ERROR : the posData is not castable to a float array."<<std::endl;
					computeIndexesFromBlocks(range, posData->getArray(), posData->getNbItems(), rangeItems_);
					computeRanges = true;
				}
				else
				{
					std::cerr<<"ERROR : The field \""<<it->first<<"\" is not splitable by "
					        <<"block and the container doesn't have a field with the flag ZCURVE_KEY."<<std::endl;
					return;
				}
			}

			//splitFields = getBaseData(it->second)->split(rangeItems, result_maps, getSplitPolicy(it->second));
			getBaseData(it->second)->split(rangeItems_, result_maps, fields, getSplitPolicy(it->second));
		}

		// Inserting the splitted field into the splitted results
		if(fields.size() != range.size())
		{
			fprintf(stderr, "ERROR : A field was not split properly."
			                " The number of chunks does not match the expected number of chunks\n");

			return;
		}

	}

	for(unsigned int i = 0; i < range.size(); i++)
	{
		buffers[i]->updateNbItems();
		buffers[i]->updateMetaData();
		//std::cout<<"Nombre d'item apres update : "<<buffers.at(i)->getNbItems()<<std::endl;
	}
	assert(buffers.size() == range.size());
	gettimeofday(&end, NULL);
	//timeGlobalSplit += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
	//return result;
	return;
}


//Todo : remove the code redundancy
bool
decaf::
//ConstructData::merge(shared_ptr<BaseData> other)
ConstructData::merge(shared_ptr<ConstructData> otherConstruct)
{
	/*std::shared_ptr<ConstructData> otherConstruct = std::dynamic_pointer_cast<ConstructData>(other);
	if(!otherConstruct)
	{
		std::cout<<"ERROR : Trying to merge two objects which have not the same dynamic type"<<std::endl;
		return false;
	}*/

	//No data yet, we simply copy the data from the other map
	if(container_->empty())
	{
		container_ = otherConstruct->container_;

		//The rest of the values should be updated with updateMetaData()
		//data_ = static_pointer_cast<void>(container_);
		/*nbItems_ = otherConstruct->nbItems_;
		nbFields_ = otherConstruct->nbFields_;
		bZCurveKey_ = otherConstruct->bZCurveKey_;
		zCurveKey_ = otherConstruct->zCurveKey_;
		bZCurveIndex_ = otherConstruct->bZCurveIndex_;
		zCurveIndex_ = otherConstruct->zCurveIndex_;
		bCountable_ = otherConstruct->bCountable_;
		bPartialCountable_ = otherConstruct->bPartialCountable_;*/
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
			if( !getBaseData(otherIt->second)->canMerge(getBaseData(it->second)) )
				return false;
		}

		//TODO : Add a checking on the merge policy and number of items in case of a merge

		//We have done all the checking, now we can merge securely
		if(merge_order_.size() > 0) //We have a priority list
		{
			std::cout<<"Merging with a specific order."<<std::endl;
			for(unsigned int i = 0; i < merge_order_.size(); i++)
			{
				std::cout<<"Merging the field "<<merge_order_.at(i).c_str()<<"..."<<std::endl;
				std::map<std::string, datafield>::iterator dataLocal
				        = container_->find(merge_order_.at(i));
				std::map<std::string, datafield>::iterator dataOther
				        = otherConstruct->getMap()->find(merge_order_.at(i));

				if(dataLocal == container_->end() || dataOther == otherConstruct->getMap()->end())
				{
					std::cerr<<"ERROR : field \""<<merge_order_.at(i)<<"\" provided by the user to "
					        <<"merge the data not found in the map."<<std::endl;
					return false;
				}

				if(! getBaseData(dataLocal->second)->merge(getBaseData(dataOther->second),
				                                           container_,
				                                           getMergePolicy(dataOther->second)) )
				{
					std::cout<<"Error while merging the field \""<<dataLocal->first<<"\". The original map has be corrupted."<<std::endl;
					return false;
				}
				//getBaseData(dataLocal->second)->setMap(container_);
				getNbItemsField(dataLocal->second) = getBaseData(dataLocal->second)->getNbItems();
			}
		}
		else  // No priority, we merge in the field order
		{
			for(std::map<std::string, datafield>::iterator it = container_->begin();
			    it != container_->end(); it++)
			{
				std::map<std::string, datafield>::iterator otherIt = otherConstruct->getMap()->find(it->first);

				if(! getBaseData(it->second)->merge(getBaseData(otherIt->second),
				                                    container_,
				                                    getMergePolicy(otherIt->second)) )
				{
					std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has be corrupted."<<std::endl;
					return false;
				}

				// Can not give the map to a field
				// Create a loop on the shared pointer
				//getBaseData(it->second)->setMap(container_);
				getNbItemsField(it->second) = getBaseData(it->second)->getNbItems();
			}
		}
	}

	return updateMetaData();
}

bool
decaf::
ConstructData::merge(char* buffer, int size)
{
	struct timeval begin;
	struct timeval end;


	//timeGlobalSerialization += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);

	if(!data_ || container_->empty() || bEmpty_)
	{
		// Very ugly because string of the same string
		// TODO: check the memory consumption
		gettimeofday(&begin, NULL);
		in_serial_buffer_ = std::string(buffer, size);
		boost::iostreams::basic_array_source<char> device(in_serial_buffer_.data(), in_serial_buffer_.size());
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
		boost::archive::binary_iarchive ia(sout);

		fflush(stdout);
		gettimeofday(&end, NULL);

		container_->clear();

		ia >> container_;

		data_ = std::static_pointer_cast<void>(container_);
	}
	else
	{
		// TODO: check the memory consumption
		pConstructData otherContainer;
		otherContainer->merge(buffer, size);

		if(otherContainer->isEmpty())
		{
			//fprintf(stderr, "Trying to merge with an empty data model. Nothing to do.\n");
			return true;
		}

		std::shared_ptr<std::map<std::string, datafield> > other;
		//ia >> other;
		other = otherContainer->container_;

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
			if( !getBaseData(otherIt->second)->canMerge(getBaseData(it->second)))
				return false;
		}

		//TODO : Add a checking on the merge policy and number of items in case of a merge

		//We have done all the checking, now we can merge securely
		if(merge_order_.size() > 0) //We have a priority list
		{
			for(unsigned int i = 0; i < merge_order_.size(); i++)
			{
				std::map<std::string, datafield>::iterator dataLocal
				        = container_->find(merge_order_.at(i));
				std::map<std::string, datafield>::iterator dataOther
				        = other->find(merge_order_.at(i));

				if(dataLocal == container_->end() || dataOther == other->end())
				{
					std::cerr<<"ERROR : field \""<<merge_order_.at(i)<<"\" provided by the user to "
					        <<"merge the data not found in the map."<<std::endl;
					return false;
				}

				if(! getBaseData(dataLocal->second)->merge(getBaseData(dataOther->second),
				                                           container_,
				                                           getMergePolicy(dataOther->second)) )
				{
					std::cout<<"Error while merging the field \""<<dataLocal->first<<"\". The original map has be corrupted."<<std::endl;
					return false;
				}
				//getBaseData(dataLocal->second)->setMap(container_);
				getNbItemsField(dataLocal->second) = getBaseData(dataLocal->second)->getNbItems();
			}
		}
		else  // No priority, we merge in the field order
		{
			for(std::map<std::string, datafield>::iterator it = container_->begin();
			    it != container_->end(); it++)
			{
				std::map<std::string, datafield>::iterator otherIt = other->find(it->first);

				if(! getBaseData(it->second)->merge(getBaseData(otherIt->second),
				                                    container_,
				                                    getMergePolicy(otherIt->second)) )
				{
					std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has been corrupted."<<std::endl;
					return false;
				}
				//getBaseData(it->second)->setMap(container_);
				getNbItemsField(it->second) = getBaseData(it->second)->getNbItems();
			}
		}

	}

	return updateMetaData();
}

bool
decaf::
ConstructData::merge()
{
	return merge(&in_serial_buffer_[0], in_serial_buffer_.size());
}

bool
decaf::
ConstructData::mergeStoredData()
{
	//We have stored all the part, now we can merge in one time all the partial parts
	if(merge_order_.size() > 0) //We have a priority list
	{
		for(unsigned int i = 0; i < merge_order_.size(); i++)
		{
			std::map<std::string, datafield>::iterator dataLocal
			        = container_->find(merge_order_.at(i));

			if(dataLocal == container_->end())
			{
				std::cerr<<"ERROR : field \""<<merge_order_.at(i)<<"\" provided by the user to "
				        <<"merge the data not found in the map."<<std::endl;
				return false;
			}

			std::vector<std::shared_ptr<BaseConstructData> > partialFields;
			for(unsigned int i = 0; i < partialData.size(); i++)
			{
				std::map<std::string, datafield>::iterator dataOther
				        = partialData.at(i)->find(merge_order_.at(i));

				if(dataOther == partialData.at(i)->end())
				{
					std::cerr<<"ERROR : field \""<<merge_order_.at(i)<<"\" provided by the user to "
					        <<"merge the data not found in the map."<<std::endl;
					return false;
				}
				partialFields.push_back(getBaseData(dataOther->second));
			}

			if(! getBaseData(dataLocal->second)->merge(partialFields,
			                                           container_,
			                                           getMergePolicy(dataLocal->second)) )
			{
				std::cout<<"Error while merging the field \""<<dataLocal->first<<"\". The original map has be corrupted."<<std::endl;
				return false;
			}
			//getBaseData(dataLocal->second)->setMap(container_);
			getNbItemsField(dataLocal->second) = getBaseData(dataLocal->second)->getNbItems();
		}
	}
	else  // No priority, we merge in the field order
	{
		for(std::map<std::string, datafield>::iterator it = container_->begin();
		    it != container_->end(); it++)
		{
			std::vector<std::shared_ptr<BaseConstructData> > partialFields;
			for(unsigned int i = 0; i < partialData.size(); i++)
			{
				std::map<std::string, datafield>::iterator dataOther
				        = partialData.at(i)->find(it->first);

				// No need to check the return, we have done it already in unserializeAndStore()
				partialFields.push_back(getBaseData(dataOther->second));
			}
			//		unsigned int totalItems = getBaseData(it->second)->getNbItems();
			//		if(it->first == "pos")
			//			std::cout<<"Nombre d'items dans le moule de base : "<<totalItems<<", nombre de fields : "<<partialFields.size()<<std::endl;
			if(! getBaseData(it->second)->merge(partialFields,
			                                    container_,
			                                    getMergePolicy(it->second)) )
			{
				std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has been corrupted."<<std::endl;
				return false;
			}
			//getBaseData(it->second)->setMap(container_);
			getNbItemsField(it->second) = getBaseData(it->second)->getNbItems();
			/*		if(it->first == "pos")
		{
			std::cout<<"Merging "<<it->first<<" with "<<partialFields[0]->getNbItems()<<" items."<<std::endl;
			for(unsigned int i = 0 ; i < partialFields.size(); i++)
				totalItems += partialFields[i]->getNbItems();
			std::cout<<"Somme theorique apres merge : "<<totalItems<<std::endl;
			std::cout<<"Somme effective apres merge : "<<getBaseData(it->second)->getNbItems()<<std::endl;
		}
*/
		}
	}

	return  updateMetaData();
}

void
decaf::
ConstructData::unserializeAndStore(char* buffer, int bufferSize)
{
	struct timeval begin;
	struct timeval end;


	//timeGlobalSerialization += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);

	// If we don't have data yet, we directly unserialize on the local map
	if(!data_ || container_->empty() || bEmpty_)
	{
		gettimeofday(&begin, NULL);
		//in_serial_buffer_ = std::string(&in_serial_buffer_[0], in_serial_buffer_.size());
		boost::iostreams::basic_array_source<char> device(buffer, bufferSize);
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
		boost::archive::binary_iarchive ia(sout);

		fflush(stdout);
		gettimeofday(&end, NULL);

		ia >> container_;

		data_ = std::static_pointer_cast<void>(container_);

		updateMetaData();
	}
	// Otherwise unserialize and store the map. To be merged later on
	else
	{
		pConstructData otherContainer;
		otherContainer->merge(buffer, bufferSize);

		if(otherContainer->isEmpty())
		{
			//fprintf(stderr, "Trying to merge with an empty data model. Nothing to do.\n");
			return;
		}

		std::shared_ptr<std::map<std::string, datafield> > other;
		//ia >> other;
		other = otherContainer->container_;


		//We check that we can merge all the fields before merging
		if(container_->size() != other->size())
		{
			std::cout<<"Error : the map don't have the same number of field. Merge aborted."<<std::endl;
			return;
		}

		for(std::map<std::string, datafield>::iterator it = container_->begin();
		    it != container_->end(); it++)
		{
			std::map<std::string, datafield>::iterator otherIt = other->find(it->first);
			if( otherIt == other->end())
			{
				std::cout<<"Error : The field \""<<it->first<<"\" is present in the"
				        <<"In the original map but not in the other one. Merge aborted."<<std::endl;
				return ;
			}
			if( !getBaseData(otherIt->second)->canMerge(getBaseData(it->second)))
				return ;
		}

		// Storing for future merge.
		partialData.push_back(other);
	}
}

bool
decaf::
ConstructData::serialize()
{
	struct timeval begin;
	struct timeval end;

	gettimeofday(&begin, NULL);
	out_serial_buffer_.resize(0);
	boost::iostreams::back_insert_device<std::string> inserter(out_serial_buffer_);
	boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
	boost::archive::binary_oarchive oa(s);

	oa << container_;
	s.flush();

	gettimeofday(&end, NULL);
	//timeGlobalSerialization += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
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
	*size = out_serial_buffer_.size(); //+1 for the \n character
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
	*size = in_serial_buffer_.size(); //+1 for the \n character
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
	updateMetaData();
}


// DEPRECATED
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

	return updateMetaData();
}

bool
decaf::
ConstructData::hasData(std::string key)
{
	std::map<std::string, datafield>::iterator it;
	it = container_->find(key);
	if(it == container_->end())
		return false;
	else
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
		//fprintf(stderr, "ERROR: key %s not found.\n", key.c_str());
		return std::shared_ptr<BaseConstructData>();
	}
	else
		return getBaseData(it->second);
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
	//bSystem_ = true;
	bEmpty_ = true;
	nbSystemFields_ = 0;
	bCountable_ = true;
	bPartialCountable_ = true;

	for(std::map<std::string, datafield>::iterator it = container_->begin();
	    it != container_->end(); it++)
	{

		// SYSTEM is treated seperatly in the redistribution process
		if(!getBaseData(it->second)->isCountable() && getScope(it->second) != DECAF_SYSTEM)
		{
			bCountable_ = false;
		}

		// Checking that we can insert this data and keep spliting the data after
		// If we already have fields with several items and we insert a new field
		// with another number of items, we can't split automatically
		else if(nbItems_ > 1 && getNbItemsField(it->second) > 1 && nbItems_ != getNbItemsField(it->second))
		{
			std::cout<<"WARNING : The number of items among the countable field is incoherent."
			        <<"Split functions may give corrupted results."<<std::endl;
			bCountable_ = false;
			bPartialCountable_ = false;
		}
		else if(getScope(it->second) != DECAF_SYSTEM &&
		        getScope(it->second) != DECAF_SHARED &&
		        getNbItemsField(it->second) > 0)// We still update the number of items
			nbItems_ = getNbItemsField(it->second);

		if(getFlag(it->second) == DECAF_POS)
		{
			bZCurveKey_ = true;
			zCurveKey_ = getBaseData(it->second);
		}

		if(getFlag(it->second) == DECAF_MORTON)
		{
			bZCurveIndex_ = true;
			zCurveIndex_ = getBaseData(it->second);
		}

		if(getScope(it->second) != DECAF_SYSTEM)
			bEmpty_ = false;

		if(getScope(it->second) == DECAF_SYSTEM)
			nbSystemFields_++;

		//The field is already in the map, we don't have to test the insert
		nbFields_++;
	}

	return true;
}

void
decaf::
ConstructData::updateNbItems()
{
	for(std::map<std::string, datafield>::iterator it = container_->begin();
	    it != container_->end(); it++)
	{
		getNbItemsField(it->second) = getBaseData(it->second)->getNbItems();
	}
}

void
decaf::
ConstructData::softClean()
{
	for(std::map<std::string, datafield>::iterator it = container_->begin();
	    it != container_->end(); it++)
	{
		getBaseData(it->second)->softClean();
		getNbItemsField(it->second) = getBaseData(it->second)->getNbItems();
	}

	updateMetaData();
}

void
decaf::
ConstructData::copySystemFields(pConstructData& source)
{
	for(std::map<std::string, datafield>::iterator it = source->container_->begin();
	    it != source->container_->end(); it++)
	{
		// If fails, the field was already in place
		if(getScope(it->second) == DECAF_SYSTEM)
			container_->insert(std::pair<std::string, datafield>(it->first, it->second));
	}
}
