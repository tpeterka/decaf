#ifndef PCONSTRUCTTYPE
#define PCONSTRUCTTYPE

#include <bredala/data_model/constructtype.h>
#include <memory>


// Composition pattern

namespace decaf {

class pConstructData {

    public:
        pConstructData(bool init = true)
        {
            if(init)
                ptr_ = std::make_shared<ConstructData>();
        }

        pConstructData(std::shared_ptr<ConstructData> ptr) : ptr_(ptr){}

        virtual ConstructData* operator -> () const {
            return ptr_.get();
        }

        bool empty()
        {
            return ptr_.use_count() == 0;
        }

        void reset()
        {
            ptr_.reset();
        }

        std::shared_ptr<ConstructData> getPtr()
        {
            return ptr_;
        }

        // Matthieu : ugly to have it here
        void preallocMultiple(int nbCopies , int nbItems, std::vector<pConstructData >& result)
        {
            assert(ptr_);

            // Creating all the empty container
            for(int i = 0; i < nbCopies; i++)
            {
                result.push_back(pConstructData());
            }

            // Going through all the field and preallocating them for the new container
            for(std::map<std::string, datafield>::iterator it = ptr_->getMap()->begin(); it != ptr_->getMap()->end(); it++)
            {
                std::vector<std::shared_ptr<BaseConstructData> > emptyFields;
                getBaseData(it->second)->preallocMultiple(nbCopies, nbItems, emptyFields);

                for(int i = 0; i < nbCopies; i++)
                {

                    result.at(i)->appendData(it->first,
                                             emptyFields.at(i),
                                             getFlag(it->second),
                                             getScope(it->second),
                                             getSplitPolicy(it->second),
                                             getMergePolicy(it->second));
                }
            }
        }


    private:
        std::shared_ptr<ConstructData> ptr_;
};

}

#endif
