//---------------------------------------------------------------------------
// Implement the storage interface for local memory. This storage object
// store data in a local map accessible by the local process.
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_STORAGE_BINARYFILE
#define DECAF_STORAGE_BINARYFILE

#include <bredala/data_model/pconstructtype.h>
#include <manala/storage/storageinterface.hpp>
#include <map>
#include <set>
#include <sys/stat.h>
#include <fstream>

namespace decaf
{
    class StorageFile: public Storage
    {
    public:
        StorageFile(unsigned int buffer_max_size, unsigned int rank);

        virtual ~StorageFile(){}

        virtual bool isFull();
        virtual unsigned int getBufferSize();
        virtual bool insert(unsigned int id, pConstructData data );
        virtual void erase(unsigned int id);
        virtual bool hasData(unsigned int id);
        virtual pConstructData getData(unsigned int id);
        virtual void processCommand(FrameCommand command, unsigned int frame_id);
        virtual unsigned int getID(unsigned int index);
        virtual unsigned int getNbDataStored();

        string getFilename(unsigned int id);
        void fillFilelist(vector<pair<unsigned int, string> >& filelist);
        void initFromFilelist(vector<pair<unsigned int, string> >& filelist);


    protected:
        unsigned int rank_;
        unsigned int buffer_max_size_;
        std::map<unsigned int, string> buffer_;
        string folder_name_;
    };
} // namespace decaf

#endif
