#ifndef ARRAY_CONSTRUCT_DATA
#define ARRAY_CONSTRUCT_DATA

#include <bredala/data_model/baseconstructdata.hpp>
#include <bredala/data_model/block.hpp>

namespace decaf {

template<typename T>
class ArrayConstructData : public BaseConstructData {
public:

    ArrayConstructData(mapConstruct map = mapConstruct(), bool bCountable = true) :
        value_(nullptr), size_(0), element_per_items_(0), owner_(false), BaseConstructData(map, bCountable), capacity_(0){}

    ArrayConstructData(T* array, int size, int element_per_items, bool owner = false,
                       mapConstruct map = mapConstruct(), bool bCountable = true) :
                        value_(array), element_per_items_(element_per_items),
                        size_(size), capacity_(size), owner_(owner),
                        BaseConstructData(map, bCountable), isSegmented_(false),totalSegmentsSize_(0){}

    ArrayConstructData(T* array, int size, int element_per_items, int capacity, bool owner = false,
                       mapConstruct map = mapConstruct(), bool bCountable = true) :
                        value_(array), element_per_items_(element_per_items),
                        size_(size), capacity_(capacity), owner_(owner),
                        BaseConstructData(map, bCountable), isSegmented_(false),totalSegmentsSize_(0){}

    ArrayConstructData(std::vector<std::pair<T*, unsigned int> > segments, int element_per_items,
                       mapConstruct map = mapConstruct(), bool bCountable = true) :
                        value_(nullptr), element_per_items_(element_per_items),
                        size_(0), capacity_(0), owner_(false),
                        BaseConstructData(map, bCountable), isSegmented_(true),
                        segments_(segments)
    {
        totalSegmentsSize_ = 0;
        for(unsigned int i = 0; i < segments_.size(); i++)
            totalSegmentsSize_ += segments_[i].second;
        size_ = totalSegmentsSize_;
	capacity_ = 0;
    }

    virtual ~ArrayConstructData()
    {
        if(owner_)
        {
            delete[] value_;
        }
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(element_per_items_);
        ar & BOOST_SERIALIZATION_NVP(size_);
        ar & BOOST_SERIALIZATION_NVP(owner_);
        //ar & BOOST_SERIALIZATION_NVP(value_);
        ar & BOOST_SERIALIZATION_NVP(isSegmented_);
        int nbSegment = segments_.size();
        ar & BOOST_SERIALIZATION_NVP(nbSegment);
        ar & BOOST_SERIALIZATION_NVP(totalSegmentsSize_);

        if(!isSegmented_)
        {
            if(Archive::is_loading::value)
            {
                assert(value_ == nullptr);
                owner_ = true;
                value_ = new T[size_];
            }
            ar & boost::serialization::make_array<T>(value_, size_);
        }
        else
        {
            if(Archive::is_saving::value)
            {
                for(int i = 0; i < nbSegment; i++)
                {
                    ar & segments_[i].second;
                    ar & boost::serialization::make_array<T>(segments_[i].first, segments_[i].second);
                    //for(int j = 0; j < segments_[i].second; j++)
                    //    ar & segments_[i].first[j];
                }
            }

            if(Archive::is_loading::value)
            {
                //We deserialize in a way that we remove the segments
                owner_ = true;
                value_ = new T[totalSegmentsSize_];

                int offset = 0;
                for(int i = 0; i < nbSegment; i++)
                {
                    int sizeSegment;
                    ar & sizeSegment;
                    ar & boost::serialization::make_array<T>(value_ + offset, sizeSegment);
                    offset += sizeSegment;
                    //for(int j = 0; j < sizeSegment; j++)
                    //{
                    //    ar & value_[offset];
                    //    offset++;
                    //}
                }
		isSegmented_ = false;
            }
        }
	if(Archive::is_loading::value)
	    capacity_ = size_;
    }

    void reallocate(unsigned int size)
    {
        if(owner_) delete [] value_;
        value_ = new T[size];
        capacity_ = size;
    }

    virtual bool appendItem(std::shared_ptr<BaseConstructData> dest, unsigned int index, ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        std::shared_ptr<ArrayConstructData<T> > destT = std::dynamic_pointer_cast<ArrayConstructData<T> >(dest);
        if(!destT)
        {
            std::cout<<"ERROR : trying to append objects with different types"<<std::endl;
            return false;
        }

	switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
		if(destT->size_ + element_per_items_ > destT->capacity_)
		{
		    std::cout<<"ERROR : not enough memory available to append a new item"<<std::endl;
		    return false;
		}
		memcpy(destT->value_ + destT->size_, value_ + index * element_per_items_, element_per_items_ * sizeof(T));
		destT->size_ += element_per_items_;
                return true;
                break;
            }
	    case DECAF_MERGE_APPEND_VALUES:
	    {
		if(destT->size_ + element_per_items_ > destT->capacity_)
                {
                    std::cout<<"ERROR : not enough memory available to append a new item"<<std::endl;
                    return false;
                }
                memcpy(destT->value_ + destT->size_, value_ + index * element_per_items_, element_per_items_ * sizeof(T));
                destT->size_ += element_per_items_;
                return true;
                break;
	    }
	    default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for vector data."<<std::endl;
                return false;
                break;
            }
	}	
        return false;
    }

    virtual void preallocMultiple(int nbCopies , int nbItems, std::vector<std::shared_ptr<BaseConstructData> >& result)
    {
        for(unsigned int i = 0; i < nbCopies; i++)
        {
	    T* newArray = new T[nbItems * element_per_items_];	
            result.push_back(std::make_shared<ArrayConstructData<T> >(
					newArray,
					0, // size
					element_per_items_, 
					nbItems * element_per_items_, // capacity
					true));
        }
    }

    virtual bool isBlockSplitable(){ return false; }

    virtual T* getArray(){ segmentsToArray(); return value_; }

    virtual int getNbItems(){ return size_ / element_per_items_; }

	virtual std::string getTypename(){ return std::string("Array_" + boost::typeindex::type_id<T>().pretty_name()); }

    int getSize(){ return size_; }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
          const std::vector<int>& range,
          std::vector< mapConstruct >& partial_map,
          ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
                //Sanity check
                int totalRange = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                    totalRange+= range[i];
                if(totalRange != getNbItems()){
                    std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                             <<") does not match the number of items of the object ("
                             <<getNbItems()<<")"<<std::endl;
                    return result;
                }

                unsigned int offset = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    T* array = new T[range[i]*element_per_items_];
                    memcpy(array, value_ + offset, range[i]*element_per_items_ * sizeof(T));
                    std::shared_ptr<ArrayConstructData<T> > sub =
                            std::make_shared<ArrayConstructData<T> >(array, range[i]*element_per_items_, element_per_items_, true);
                    offset  += (range[i]*element_per_items_);
                    result.push_back(sub);
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                //The new subarray can't be the owner. The array will be deleted
                //when this is destroyed is this is the owner or the user will so it if
                //this is not the owner
                std::shared_ptr<ArrayConstructData> sub = std::make_shared<ArrayConstructData>(
                            value_, size_, element_per_items_, false);
                result.push_back(sub);
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for ArrayConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual void split(
          const std::vector<int>& range,
          std::vector< mapConstruct >& partial_map,
          std::vector<std::shared_ptr<BaseConstructData> >& fields,
          ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
                //Sanity check
                int totalRange = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                    totalRange+= range[i];
                if(totalRange > getNbItems()){
                    std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                             <<") does not match the number of items of the object ("
                             <<getNbItems()<<")"<<std::endl;
                    return ;
                }

                unsigned int offset = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                {

                    // TODO : use the fields as memory layer
                    //T* array = new T[range[i].back() * element_per_items_];
                    std::shared_ptr<ArrayConstructData<T> > arrayconstruct =
                            std::dynamic_pointer_cast<ArrayConstructData<T> >(fields[i]);
                    if(!arrayconstruct)
                    {
                        std::cout<<"ERROR : dynamic cast into an array failed during a split."<<std::endl;
                        return;
                    }

                    //Note enough space, we have to reallocate
                    if(arrayconstruct->capacity_ < range[i] * element_per_items_)
                        arrayconstruct->reallocate(range[i] * element_per_items_);


                    T* array = arrayconstruct->getArray();

                    memcpy(array, value_ + offset, range[i] * element_per_items_ * sizeof(T));
                    offset  += (range[i]*element_per_items_);

                    arrayconstruct->size_ = range[i] * element_per_items_;
                }
                break;
            }

            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for ArrayConstructData"<<std::endl;
                break;
            }
        }
        return ;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
                //Sanity check
                int totalRange = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                    totalRange+= range[i].size();
                if(totalRange != getNbItems()){
                    std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                             <<") does not match the number of items of the object ("
                             <<getNbItems()<<")"<<std::endl;
                    return result;
                }

                //typename std::vector<T>::iterator it = value_.begin();
                //unsigned int offset = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    //T* array = new T[range[i].size() * element_per_items_];
                    T* array = new T[range[i].back() * element_per_items_];
                    
		    /*unsigned int offset = 0;
                    for(unsigned int j = 0; j< range[i].size(); j++)
                    {
                        //temp.insert( temp.end(),
                        //             it+(range[i][j]*element_per_items_),
                        //             it+((range[i][j]+1)*element_per_items_)
                        //             );
                        memcpy(array + offset,
                               value_ + range[i][j]*element_per_items_,
                               element_per_items_ * sizeof(T));
                        offset += element_per_items_;
                    }*/

		    //Working version with segmented copy
                    /*unsigned int offsetDestArray = 0;
                    unsigned int nbCopy = 0;
                    
                    while(offsetDestArray < range[i].size())
                    {
                        int currentTransationSize =  1;

                        //Computing how many consecutive items are in the range
                        while(offsetDestArray + currentTransationSize < range[i].size() - 1 && range[i].at(offsetDestArray+currentTransationSize-1)+1 == range[i].at(offsetDestArray+currentTransationSize))
                        {
                            currentTransationSize++;
                        }

                        //Now we can copy a chunk
                        memcpy(array + offsetDestArray * element_per_items_,
                               value_ + range[i].at(offsetDestArray)*element_per_items_,
                               currentTransationSize * element_per_items_ * sizeof(T));
                        offsetDestArray += currentTransationSize;
                        nbCopy++;
			//break;
                    }*/

		    //Test version with the vector 
		    unsigned int nbCopy = 0;
		    unsigned int offsetDestArray = 0;

		    for(unsigned int j = 0; j < range[i].size() - 1; j++)
	            {
			memcpy(array + offsetDestArray * element_per_items_,
			value_ + range[i][j] * element_per_items_,
			range[i].at(j+1) * element_per_items_ * sizeof(T));
			offsetDestArray+= range[i].at(j+1);
			j++; // The second j contains the number of items of this segment
			nbCopy++;
		    }

                    //printf("We did %u copies instead of %u\n", nbCopy, range[i].size());

                    std::shared_ptr<ArrayConstructData<T> > sub =
                    //        std::make_shared<ArrayConstructData<T> >(array, range[i].size() * element_per_items_,
                    //                                                 element_per_items_, true);
                    	    std::make_shared<ArrayConstructData<T> >(array, range[i].back() * element_per_items_,
								     element_per_items_, true);
		    //        std::make_shared<ArrayConstructData<T> >(array, currentTransationSize * element_per_items_,
                    //                                                 element_per_items_, true);
                    result.push_back(sub);
                }
                break;
            }
            case DECAF_SPLIT_SEGMENTED: // TODO : readapt the segments to
            {
                // TODO : remake the segments so it reflects the new meaning of the ranges
                std::cerr<<"ERROR : DECAF_SPLIT_SEGMENTED in not implemented for split by range"<<std::endl;
                return result;


                //typename std::vector<T>::iterator it = value_.begin();
                //unsigned int offset = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    //T* array = new T[range[i].size() * element_per_items_];

                    unsigned int offsetDestArray = 0;
                    unsigned int nbCopy = 0;
                    std::vector<std::pair<T*, unsigned int> > segments;
                    unsigned int totalSegmentsSize_;

                    while(offsetDestArray < range[i].size())
                    {
                        int currentTransationSize =  1;

                        //Computing how many consecutive items are in the range
                        while(offsetDestArray + currentTransationSize < range[i].size() - 1 && range[i].at(offsetDestArray+currentTransationSize-1)+1 == range[i].at(offsetDestArray+currentTransationSize))
                        {
                            currentTransationSize++;
                        }

                        //Now we can copy a chunk
                        //memcpy(array + offsetDestArray * element_per_items_,
                        //       value_ + range[i].at(offsetDestArray)*element_per_items_,
                        //       currentTransationSize * element_per_items_ * sizeof(T));
                        //offsetDestArray += currentTransationSize;
                        //nbCopy++;
                        std::pair<T*, unsigned int> segment(
                                    value_ + range[i].at(offsetDestArray)*element_per_items_,
                                    currentTransationSize * element_per_items_);
                        totalSegmentsSize_ += currentTransationSize * element_per_items_;
                        offsetDestArray += currentTransationSize;
                        nbCopy++;
                        segments.push_back(segment);
                        //break;
                    }

                    //printf("We did %u segments instead of %u\n", nbCopy, range[i].size());

                    std::shared_ptr<ArrayConstructData<T> > sub =
                            std::make_shared<ArrayConstructData<T> >(segments,
                                                                     element_per_items_);
                    //        std::make_shared<ArrayConstructData<T> >(array, currentTransationSize * element_per_items_,
                    //                                                 element_per_items_, true);
                    result.push_back(sub);
                }
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for ArrayConstructData"<<std::endl;
                break;
            }
        }

        assert(result.size() == range.size());
        return result;
    }

    //This function should never be called
    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< Block<3> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        std::cout<<"ERROR : Method split by block is not implemented in ArrayConstruct class."<<std::endl;
        exit(0);
    }

    virtual void split(
	    const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {

	switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
		for(unsigned int i = 0; i < range.size(); i++)
                {
                    // TODO : use the fields as memory layer
                    //T* array = new T[range[i].back() * element_per_items_];
                    std::shared_ptr<ArrayConstructData<T> > arrayconstruct =
                            std::dynamic_pointer_cast<ArrayConstructData<T> >(fields[i]);
		    if(!arrayconstruct)
		    {
			std::cout<<"ERROR : dynamic cast into an array failed during a split."<<std::endl;
			return;
		    } 

		    //Note enough space, we have to reallocate
                    if(arrayconstruct->capacity_ < range[i].back() * element_per_items_)
                        arrayconstruct->reallocate(range[i].back() * element_per_items_);


                    T* array = arrayconstruct->getArray();
		    unsigned int nbCopy = 0;
                    unsigned int offsetDestArray = 0;

                    for(unsigned int j = 0; j < range[i].size() - 1; j++) //-1 because the last element of range is the total size of the ranges
                    {
                        memcpy(array + offsetDestArray * element_per_items_,
                        value_ + range[i][j] * element_per_items_,
                        range[i][j+1] * element_per_items_ * sizeof(T));
                        offsetDestArray+= range[i][j+1];
                        j++; // The second j contains the number of items of this segment
                        nbCopy++;
                    }
		    //std::shared_ptr<ArrayConstructData<T> > sub =
		    //	std::make_shared<ArrayConstructData<T> >(array, range[i].back() * element_per_items_,
                    //                                             element_per_items_, true);
		    //result.push_back(sub);
                    arrayconstruct->size_ = range[i].back() * element_per_items_;

                }
                break;
            }
	    default:
            {
                std::cout<<"Policy "<<policy<<" not supported for ArrayConstructData"<<std::endl;
                break;
            }
	}
	return;
    }

    virtual bool merge( std::shared_ptr<BaseConstructData> other,
                        mapConstruct partial_map,
                        ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        std::shared_ptr<ArrayConstructData<T> > other_ = std::dynamic_pointer_cast<ArrayConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                T* newArray = new T[size_ + other_->size_];
                memcpy(newArray, value_, size_ * sizeof(T));
                memcpy(newArray + size_, other_->value_, other_->size_ * sizeof(T));

                if(owner_)
                    delete[] value_;
                value_ = newArray;
                size_ = size_ + other_->size_;
                capacity_ = size_;
                owner_ = true;
                return true;
                break;
            }
            case DECAF_MERGE_FIRST_VALUE: //We don't have to do anything here
            {
                return true;
                break;
            }
            case DECAF_MERGE_APPEND_VALUES:
            {
                T* newArray = new T[size_ + other_->size_];
                memcpy(newArray, value_, size_ * sizeof(T));
                memcpy(newArray + size_, other_->value_, other_->size_ * sizeof(T));

                if(owner_) delete[] value_;
                value_ = newArray;
                size_ = size_ + other_->size_;
                capacity_ = size_;
                owner_ = true;
                return true;
                break;
            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for simple data."<<std::endl;
                return false;
                break;
            }
        }
    }

    virtual bool merge(std::vector<std::shared_ptr<BaseConstructData> >& others,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
	// First we cast all the arrays and some all the size
	unsigned int totalSize = size_;
	std::vector<std::shared_ptr<ArrayConstructData<T> > > partials;
	for(unsigned int i = 0; i < others.size(); i++)
	{
		std::shared_ptr<ArrayConstructData<T> >other = std::dynamic_pointer_cast<ArrayConstructData<T> >(others[i]);
		if(!other) std::cerr<<"ERROR : the conversion failed."<<std::endl;
		totalSize += other->size_;
		partials.push_back(other);
	}	

	//std::cerr<<"Total size after merge : "<<totalSize<<std::endl;
	switch(policy)
        {
            case DECAF_MERGE_FIRST_VALUE: //We don't have to do anything here
            {
                return true;
                break;
            }
            case DECAF_MERGE_APPEND_VALUES:
            {
		T* newArray = new T[totalSize];
                memcpy(newArray, value_, size_ * sizeof(T));
		unsigned int offset = size_;
		for(unsigned int i = 0; i < partials.size(); i++)
		{
                    memcpy(newArray + offset, partials[i]->value_, partials[i]->size_ * sizeof(T));
		    offset += partials[i]->size_;
		}

                if(owner_) delete[] value_;
                value_ = newArray;
                size_ = totalSize;
                capacity_ = size_;
                owner_ = true;
		return true;
		break;
	    }
	    default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for array data with multiple merges."<<std::endl;
                return false;
                break;
            }
        }
    }

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other)
    {
        std::shared_ptr<ArrayConstructData<T> >other_ = std::dynamic_pointer_cast<ArrayConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }



protected:
    T* value_;
    int element_per_items_; //One semantic item is composed of element_per_items_ items in the vector
    int size_;
    unsigned int capacity_;
    bool owner_;

    bool isSegmented_;
    std::vector<std::pair<T*, unsigned int> > segments_;
    unsigned int totalSegmentsSize_;

    void segmentsToArray()
    {
        if(!segments_.empty())
        {
            if(value_ && owner_)
                delete [] value_;
            value_ = new T[totalSegmentsSize_];
            size_ = totalSegmentsSize_;

            unsigned int offset = 0;
            for(unsigned int i = 0; i < segments_.size(); i++)
            {
                memcpy(value_ + offset,
                       segments_[i].first,
                       segments_[i].second * sizeof(T));
                offset+= segments_[i].second;
            }


        }
    }

    virtual void softClean()
    {
	size_ = 0; // We don't delete the data for now
    }
};

} //namespace

#endif

