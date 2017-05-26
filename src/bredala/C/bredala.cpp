#include <bredala/C/bredala.h>

////////////////////////////////////////
// Standard includes and namespaces.
#include <memory>
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

////////////////////////////////////////
// Decaf includes and namespaces.

#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/blockfield.hpp>
#include <bredala/transport/redist_comp.h>
#include <bredala/transport/mpi/redist_block_mpi.h>
#include <bredala/transport/mpi/redist_count_mpi.h>
#include <bredala/transport/mpi/redist_round_mpi.h>
#include <bredala/transport/mpi/redist_zcurve_mpi.h>
#include <bredala/transport/mpi/redist_proc_mpi.h>
#include <bredala/data_model/boost_macros.h>

namespace
{

using namespace std;
using namespace decaf;
        //--------------------------------------
        // Generic utilities.

        static
        void
        not_implemented()
        {
                std::cerr << "Not implemented.\n";
                std::abort();
        }

        static
        void
        not_allowed(std::string msg) {
                std::cerr << msg << std::endl;
        }

        struct Base
        {
                virtual ~Base()
                {}
        };
        /*
        class SimpleField : public Base
        {

        public:
            SimpleField(void* data, bca_type type)
            {
                assert(data != NULL);

                switch(type)
                {
                    case bca_CHAR:
                    {
                        ptr_ = make_shared<SimpleConstructData<char>(*data);
                        break;
                    }
                    case bca_FLOAT:
                    {
                        ptr_ = make_shared<SimpleConstructData<float>(*data);
                        break;
                    }
                    case bca_DOUBLE:
                    {
                        ptr_ = make_shared<SimpleConstructData<double>(*data);
                        break;
                    }
                    case bca_INT:
                    {
                        ptr_ = make_shared<SimpleConstructData<int>(*data);
                        break;
                    }
                    case bca_UNSIGNED:
                    {
                        ptr_ = make_shared<SimpleConstructData<unsigned int>(*data);
                        break;
                    }
                    default:
                        cerr<<"ERROR : unsupport data type ("<<type<<") when creating a simple field."<<endl;
                }
            }

            ~SimpleField(){}

            int getNbItems()
            {
                if(ptr_)
                    return ptr_->getNbItems();
                else
                    return 0;
            }

            bool getData(bca_type type, void* data)
            {
                switch(type)
                {
                    case bca_CHAR:
                    {
                        shared_ptr<SimpleConstructData<char> > ptr =
                                dynamic_pointer_cast<SimpleConstructData<char> >(ptr_);
                        if(ptr)
                        {
                            *data = ptr->getData();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_FLOAT:
                    {
                        shared_ptr<SimpleConstructData<float> > ptr =
                                dynamic_pointer_cast<SimpleConstructData<float> >(ptr_);
                        if(ptr)
                        {
                            *data = ptr->getData();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_DOUBLE:
                    {
                        shared_ptr<SimpleConstructData<double> > ptr =
                                dynamic_pointer_cast<SimpleConstructData<double> >(ptr_);
                        if(ptr)
                        {
                            *data = ptr->getData();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_INT:
                    {
                        shared_ptr<SimpleConstructData<int> > ptr =
                                dynamic_pointer_cast<SimpleConstructData<int> >(ptr_);
                        if(ptr)
                        {
                            *data = ptr->getData();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_UNSIGNED:
                    {
                        shared_ptr<SimpleConstructData<unsigned int> > ptr =
                                dynamic_pointer_cast<SimpleConstructData<unsigned int> >(ptr_);
                        if(ptr)
                        {
                            *data = ptr->getData();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    default:
                    {
                        cerr<<"ERROR : unsupport data type ("<<type<<") when creating a simple field."<<endl;
                    }
                }

                return false;
            }

        private:
            bca_type type_;
            shared_ptr<BaseConstructData> ptr_;
        };

        class ArrayField : public Base
        {

        public:
            ArrayField(void* data,
                       bca_type type,
                       int size,
                       int element_per_items,
                       int capacity,
                       bool owner)
            {
                assert(data != NULL);

                switch(type)
                {
                    case bca_CHAR:
                    {
                        ptr_ = make_shared<ArrayConstructData<char> >(data, size, element_per_items, capacity, owner);
                        break;
                    }
                    case bca_FLOAT:
                    {
                        ptr_ = make_shared<ArrayConstructData<float> >(data, size, element_per_items, capacity, owner);
                        break;
                    }
                    case bca_DOUBLE:
                    {
                        ptr_ = make_shared<ArrayConstructData<double> >(data, size, element_per_items, capacity, owner);
                        break;
                    }
                    case bca_INT:
                    {
                        ptr_ = make_shared<ArrayConstructData<int> >(data, size, element_per_items, capacity, owner);
                        break;
                    }
                    case bca_UNSIGNED:
                    {
                        ptr_ = make_shared<ArrayConstructData<unsigned int> >(data, size, element_per_items, capacity, owner);
                        break;
                    }
                    default:
                    {
                        cerr<<"ERROR : unsupport data type ("<<type<<") when creating a simple field."<<endl;
                    }
                }
            }

            ~ArrayField(){}

            int getNbItems()
            {
                if(ptr_)
                    return ptr_->getNbItems();
                else
                    return 0;
            }

            bool getArray(bca_type type, size_t* size, void* data)
            {
                if(!ptr_)
                    return false;
                switch(type)
                {
                    case bca_CHAR:
                    {
                        shared_ptr<ArrayConstructData<char> > ptr =
                                dynamic_pointer_cast<ArrayConstructData<char> >(ptr_);
                        if(ptr)
                        {
                            data = ptr->getArray();
                            *size = ptr->getSize();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_FLOAT:
                    {
                        shared_ptr<ArrayConstructData<float> > ptr =
                                dynamic_pointer_cast<ArrayConstructData<float> >(ptr_);
                        if(ptr)
                        {
                            data = ptr->getArray();
                            *size = ptr->getSize();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_DOUBLE:
                    {
                        shared_ptr<ArrayConstructData<double> > ptr =
                                dynamic_pointer_cast<ArrayConstructData<double> >(ptr_);
                        if(ptr)
                        {
                            data = ptr->getArray();
                            *size = ptr->getSize();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_INT:
                    {
                        shared_ptr<ArrayConstructData<int> > ptr =
                                dynamic_pointer_cast<ArrayConstructData<int> >(ptr_);
                        if(ptr)
                        {
                            data = ptr->getArray();
                            *size = ptr->getSize();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    case bca_UNSIGNED:
                    {
                        shared_ptr<ArrayConstructData<unsigned int> > ptr =
                                dynamic_pointer_cast<ArrayConstructData<unsigned int> >(ptr_);
                        if(ptr)
                        {
                            data = ptr->getArray();
                            *size = ptr->getSize();
                            return true;
                        }
                        else
                            return false;
                        break;
                    }
                    default:
                    {
                        cerr<<"ERROR : unsupport data type ("<<type<<") when creating a simple field."<<endl;
                        break;
                    }
                }

                return false;
            }


        private:
            bca_type type_;
            std::shared_ptr<BaseConstructData> ptr_;
        };

        class Construct : public Base
        {
        public:
            Construct()
            {
                ptr_ = make_shared<ConstructData>();
            }

            ~Construct(){}
        private:
            shared_ptr<ConstructData> ptr_;
        };

        */

        //--------------------------------------
        // Traits which associate C++ types to their C wrappers and vice versa.

        template <typename T1>
        struct box_traits
        {};

#	define BOX_TRAITS_1(BOX, ITEM) \
        template <> \
        struct box_traits<BOX> \
        { \
                typedef ITEM item; \
        }
#	define BOX_TRAITS_2(BOX, ITEM) \
        template <> \
        struct box_traits<ITEM> \
        { \
                typedef BOX box; \
        }
#	define BOX_TRAITS(BOX, ITEM) BOX_TRAITS_1(BOX, ITEM); BOX_TRAITS_2(BOX, ITEM)

        /* Here we  associate the  type of  the (C) box  to the  corresponding (C++)
         * object and vice versa. */
        BOX_TRAITS(bca_constructdata, pConstructData);
        BOX_TRAITS(bca_field, BaseField);
        BOX_TRAITS(bca_redist, RedistComp);

        /* For derivated classes we only associate them with the boxes of their base
         * class. */
        BOX_TRAITS_2(bca_field, ArrayFieldf);
        BOX_TRAITS_2(bca_field, ArrayFieldd);
        BOX_TRAITS_2(bca_field, ArrayFieldi);
        BOX_TRAITS_2(bca_field, ArrayFieldu);
        BOX_TRAITS_2(bca_field, ArrayField<char>);
        BOX_TRAITS_2(bca_field, SimpleFieldf);
        BOX_TRAITS_2(bca_field, SimpleFieldd);
        BOX_TRAITS_2(bca_field, SimpleFieldi);
        BOX_TRAITS_2(bca_field, SimpleFieldu);
        BOX_TRAITS_2(bca_field, SimpleField<char>);
        BOX_TRAITS_2(bca_field, BlockField);
        BOX_TRAITS_2(bca_redist, RedistBlockMPI);
        BOX_TRAITS_2(bca_redist, RedistZCurveMPI);
        BOX_TRAITS_2(bca_redist, RedistRoundMPI);
        BOX_TRAITS_2(bca_redist, RedistCountMPI);
        BOX_TRAITS_2(bca_redist, RedistProcMPI);

#	undef BOX_TRAITS
#	undef BOX_TRAITS_2
#	undef BOX_TRAITS_1

        //--------------------------------------
        // Utilities to convert C++ objects to their C wrappers (box) and vice versa
        // (unbox).

        template <typename T>
        static
        typename box_traits<T>::box
        box(T *item)
        {
                return reinterpret_cast<typename box_traits<T>::box>(item);
        }

        template <typename T>
        static
        typename box_traits<T>::item *
        unbox(T box)
        {
                return reinterpret_cast<typename box_traits<T>::item *>(box);
        }

        template <typename Item, typename Box>
        static
        Item *
        unbox(Box box)
        {
                assert(dynamic_cast<Item *>(reinterpret_cast<Base *>(box)) == reinterpret_cast<Item *>(box));
                return reinterpret_cast<Item *>(box);
        }
} // namespace

////////////////////////////////////////
// C wrappers.

extern "C"
{
    bca_constructdata
    bca_create_constructdata()
    {
        return box(new pConstructData());
    }

    bool
    bca_append_field(   bca_constructdata container,
                        const char* name,
                        bca_field field,
                        bca_ConstructTypeFlag flag,
                        bca_ConstructTypeScope scope,
                        bca_ConstructTypeSplitPolicy split,
                        bca_ConstructTypeMergePolicy merge)
    {
        pConstructData* container_ = unbox(container);
        BaseField* field_ = unbox(field);
        return (*container_)->appendData(
                    name,
                    *field_,
                    (decaf::ConstructTypeFlag)flag,
                    (decaf::ConstructTypeScope)scope,
                    (decaf::ConstructTypeSplitPolicy)split,
                    (decaf::ConstructTypeMergePolicy)merge);
    }

    void bca_free_constructdata(bca_constructdata data)
    {
        delete unbox(data);
    }

    void bca_free_field(bca_field data)
    {
        delete (unbox(data));
    }


    bca_field
    bca_get_simplefield(bca_constructdata container, const char* name, bca_type type)
    {
        pConstructData* container_ = unbox(container);
        switch(type)
        {
        case bca_INT:
        {
            SimpleFieldi field = (*container_)->getFieldData<SimpleFieldi>(
                        name);
            if(field)
                return box(new SimpleFieldi(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_FLOAT:
        {
            SimpleFieldf field = (*container_)->getFieldData<SimpleFieldf>(
                        name);
            if(field)
                return box(new SimpleFieldf(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_DOUBLE:
        {
            SimpleFieldd field = (*container_)->getFieldData<SimpleFieldd>(
                        name);
            if(field)
                return box(new SimpleFieldd(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_CHAR:
        {
            SimpleField<char> field = (*container_)->getFieldData<SimpleField<char> >(
                        name);
            if(field)
                return box(new SimpleField<char>(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_UNSIGNED:
        {
            SimpleFieldu field = (*container_)->getFieldData<SimpleFieldu>(
                        name);
            if(field)
                return box(new SimpleFieldu(field.getPtr()));
            else
                return NULL;
            break;
        }
        default:
        {
            cerr<<"Unknow type for bca_get_simplefield."<<endl;
            break;
        }
        }

        return NULL;

    }

    bca_field
    bca_get_arrayfield(bca_constructdata container, const char* name, bca_type type)
    {
        pConstructData* container_ = unbox(container);
        switch(type)
        {
        case bca_INT:
        {
            ArrayFieldi field = (*container_)->getFieldData<ArrayFieldi>(
                        name);
            if(field)
                return box(new ArrayFieldi(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_FLOAT:
        {
            ArrayFieldf field = (*container_)->getFieldData<ArrayFieldf>(
                        name);
            if(field)
                return box(new ArrayFieldf(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_DOUBLE:
        {
            ArrayFieldd field = (*container_)->getFieldData<ArrayFieldd>(
                        name);
            if(field)
                return box(new ArrayFieldd(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_CHAR:
        {
            ArrayField<char> field = (*container_)->getFieldData<ArrayField<char> >(
                        name);
            if(field)
                return box(new ArrayField<char>(field.getPtr()));
            else
                return NULL;
            break;
        }
        case bca_UNSIGNED:
        {
            ArrayFieldu field = (*container_)->getFieldData<ArrayFieldu>(
                        name);
            if(field)
                return box(new ArrayFieldu(field.getPtr()));
            else
                return NULL;
            break;
        }
        default:
        {
            cerr<<"Unknow type for bca_get_arrayfield."<<endl;
            break;
        }
        }

        return NULL;

    }

    bca_field
    bca_get_blockfield(bca_constructdata container, const char* name)
    {
         pConstructData* container_ = unbox(container);

         BlockField field = (*container_)->getFieldData<BlockField>(name);
         if(field)
             return box(new BlockField(field.getPtr()));
         else
             return NULL;
    }

    bca_field
    bca_create_simplefield(void* data,
                           bca_type type)
    {
        switch(type)
        {
        case bca_INT:
        {
            return box(new SimpleFieldi((int*)(data)));
            break;
        }
        case bca_FLOAT:
        {
            return box(new SimpleFieldf((float*)data));
            break;
        }
        case bca_DOUBLE:
        {
            return box(new SimpleFieldd((double*)data));
            break;
        }
        case bca_CHAR:
        {
            return box(new SimpleField<char>((char*)data));
            break;
        }
        case bca_UNSIGNED:
        {
            return box(new SimpleFieldu((unsigned int*)data));
            break;
        }
        default:
        {
            cerr<<"Unknow type for bca_get_simplefield."<<endl;
            break;
        }
        }
        return NULL;
    }

    bca_field
    bca_create_arrayfield(void* data,
                          bca_type type,
                          int size,
                          int element_per_items,
                          int capacity,
                          bool owner)
    {
        switch(type)
        {
        case bca_INT:
        {
            return box(new ArrayFieldi((int*)data, size, element_per_items, capacity, owner));
            break;
        }
        case bca_FLOAT:
        {
            return box(new ArrayFieldf((float*)data, size, element_per_items, capacity, owner));
            break;
        }
        case bca_DOUBLE:
        {
            return box(new ArrayFieldd((double*)data, size, element_per_items, capacity, owner));
            break;
        }
        case bca_CHAR:
        {
            return box(new ArrayField<char>((char*)data, size, element_per_items, capacity, owner));
            break;
        }
        case bca_UNSIGNED:
        {
            return box(new ArrayFieldu((unsigned int*)data, size, element_per_items, capacity, owner));
            break;
        }
        default:
        {
            cerr<<"Unknow type for bca_get_simplefield."<<endl;
            break;
        }
        }
        return NULL;
    }

    bca_field
    bca_create_blockfield(bca_block *block)
    {
        // We initiate manually
        BlockField* newBlock = new BlockField(true);
        newBlock->getBlock()->setGridspace(block->gridspace);
        if(block->globalbbox != NULL)
        {
            vector<float> box(block->globalbbox, block->globalbbox + 6);
            newBlock->getBlock()->setGlobalBBox(box);
        }
        if(block->globalextends != NULL)
        {
            vector<unsigned int> box(block->globalextends, block->globalextends + 6);
            newBlock->getBlock()->setGlobalExtends(box);
        }
        if(block->localbbox != NULL)
        {
            vector<float> box(block->localbbox, block->localbbox + 6);
            newBlock->getBlock()->setLocalBBox(box);
        }
        if(block->localextends != NULL)
        {
            vector<unsigned int> box(block->localextends, block->localextends + 6);
            newBlock->getBlock()->setLocalExtends(box);
        }

        if(block->ghostsize > 0 && block->localbbox != NULL && block->localextends != NULL)
            newBlock->getBlock()->buildGhostRegions(block->ghostsize);

        return box(newBlock);
    }

    bool
    bca_get_data(bca_field field, bca_type type, void* data)
    {
        BaseField* basefield = unbox(field);
        assert(basefield != NULL);
        switch(type)
        {
        case bca_INT:
        {
            SimpleFieldi *dfield = dynamic_cast<SimpleFieldi*>(basefield);
            assert(dfield != NULL);

            int* out = (int*)data;
            *out = dfield->getData();

            return true;
        }
        case bca_FLOAT:
        {
            SimpleFieldf *dfield = dynamic_cast<SimpleFieldf*>(basefield);
            assert(dfield != NULL);

            float* out = (float*)data;
            *out = dfield->getData();

            return true;
        }
        case bca_DOUBLE:
        {
            SimpleFieldd *dfield = dynamic_cast<SimpleFieldd*>(basefield);
            assert(dfield != NULL);

            double* out = (double*)data;
            *out = dfield->getData();

            return true;
        }
        case bca_CHAR:
        {
            SimpleField<char> *dfield = dynamic_cast<SimpleField<char>* >(basefield);
            assert(dfield != NULL);

            char* out = (char*)data;
            *out = dfield->getData();

            return true;
        }
        case bca_UNSIGNED:
        {
            SimpleFieldu *dfield = dynamic_cast<SimpleFieldu*>(basefield);
            assert(dfield != NULL);

            unsigned int* out = (unsigned int*)data;
            *out = dfield->getData();

            return true;
        }
        default:
        {
            cerr<<"Unknow type for bca_get_data."<<endl;
            break;
        }
        }

        return false;
    }


    void*
    bca_get_array(bca_field field, bca_type type, size_t* size)
    {
        BaseField* basefield = unbox(field);
        assert(basefield != NULL);
        switch(type)
        {
        case bca_INT:
        {
            ArrayFieldi *dfield = dynamic_cast<ArrayFieldi*>(basefield);
            assert(dfield != NULL);

            *size = dfield->getPtr()->getSize();
            return dfield->getArray();
        }
        case bca_FLOAT:
        {
            ArrayFieldf *dfield = dynamic_cast<ArrayFieldf*>(basefield);
            assert(dfield != NULL);

            *size = dfield->getPtr()->getSize();
            return dfield->getArray();
        }
        case bca_DOUBLE:
        {
            ArrayFieldd *dfield = dynamic_cast<ArrayFieldd*>(basefield);
            assert(dfield != NULL);

            *size = dfield->getPtr()->getSize();
            return dfield->getArray();
        }
        case bca_CHAR:
        {
            ArrayField<char> *dfield = dynamic_cast<ArrayField<char>* >(basefield);
            assert(dfield != NULL);

            *size = dfield->getPtr()->getSize();
            return dfield->getArray();
        }
        case bca_UNSIGNED:
        {
            ArrayFieldu *dfield = dynamic_cast<ArrayFieldu*>(basefield);
            assert(dfield != NULL);

            *size = dfield->getPtr()->getSize();
            return dfield->getArray();
        }
        default:
        {
            cerr<<"Unknow type for bca_get_array."<<endl;
            break;
        }
        }
        *size = 0;
        return NULL;
    }

    bool bca_get_block(bca_field field, bca_block* block)
    {
        assert(block != NULL);

        BaseField* basefield = unbox(field);
        BlockField* dfield = dynamic_cast<BlockField*>(basefield);

        assert(dfield);

        Block<3>* obj =  dfield->getBlock();

        block->ghostsize = obj->getGhostsize();
        block->gridspace=  obj->getGridspace();
        block->globalbbox = obj->getGlobalBBox();
        block->globalextends = obj->getGlobalExtends();
        block->localbbox = obj->getLocalBBox();
        block->localextends = obj->getLocalExtends();
        block->ownbbox = obj->getOwnBBox();
        block->ownextends = obj->getOwnExtends();
        return true;
    }

    int
    bca_get_nbitems_field(bca_field field)
    {
        BaseField* data = unbox(field);
        return data->getNbItems();
    }

    int
    bca_get_nbitems_constructdata(bca_constructdata container)
    {
        pConstructData* cont = unbox(container);
        return cont->getPtr()->getNbItems();
    }

    bool
    bca_split_by_range(bca_constructdata container, int nb_range, int* ranges, bca_constructdata* results)
    {
        pConstructData* cont = unbox(container);
        vector<int> ranges_v(ranges, ranges + nb_range);

        vector<std::shared_ptr<BaseData> > chunks = (*cont)->split(ranges_v);

        assert(chunks.size() == nb_range);

        for(unsigned int i = 0; i < chunks.size(); i++)
            results[i] = box(new pConstructData(dynamic_pointer_cast<ConstructData>(chunks[i])));

        return true;
    }


    bool
    bca_merge_constructdata(bca_constructdata cont1, bca_constructdata cont2)
    {
        pConstructData* container1 = unbox(cont1);
        pConstructData* container2 = unbox(cont2);

        return container1->getPtr()->merge(container2->getPtr());
    }

    bca_redist
    bca_create_redist(bca_RedistType type, int rank_source, int nb_sources, int rank_dest, int nb_dests, MPI_Comm communicator)
    {
        switch(type)
        {
        case bca_REDIST_COUNT:
        {
            return box(new RedistCountMPI(rank_source, nb_sources,
                                          rank_dest, nb_dests, communicator));
        }
        case bca_REDIST_ROUND:
        {
            return box(new RedistRoundMPI(rank_source, nb_sources,
                                          rank_dest, nb_dests, communicator));
        }
        case bca_REDIST_ZCURVE:
        {
            not_implemented();
            //return box(new RedistZCurveMPI(rank_source, nb_sources,
            //                               rank_dest, nb_dests, communicator));
            break;
        }
        case bca_REDIST_BLOCK:
        {
            return box(new RedistBlockMPI(rank_source, nb_sources,
                                          rank_dest, nb_dests, communicator));
        }
        case bca_REDIST_PROC:
        {
            return box(new RedistProcMPI(rank_source, nb_sources,
                                         rank_dest, nb_dests, communicator));
        }
        default:
        {
            cerr<<"ERROR : unknown redistribution type."<<endl;
            break;
        }
        }

        return NULL;
    }

    void bca_free_redist(bca_redist comp)
    {
        delete unbox(comp);
    }

    void
    bca_process_redist(bca_constructdata data, bca_redist comp, bca_RedistRole role)
    {
        pConstructData* container = unbox(data);
        RedistComp* component = unbox(comp);

        component->process(*container, (RedistRole)role);
    }

    void
    bca_flush_redist(bca_redist comp)
    {
        RedistComp* component = unbox(comp);

        component->flush();
    }

}
