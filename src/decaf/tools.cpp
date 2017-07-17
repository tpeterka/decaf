#include <decaf/tools.hpp>

void
decaf::all_err(int err_code)
{
        switch (err_code) {
        case DECAF_OK :
                break;
        case DECAF_COMM_SIZES_ERR :
                fprintf(stderr, "Decaf error: Group sizes of producer, consumer, and dataflow exceed total "
                        "size of world communicator\n");
                break;
        default:
                break;
        }
}

Decomposition
decaf::
stringToDecomposition(std::string name)
{
        if (name.compare(std::string("round")) == 0)
                return DECAF_ROUND_ROBIN_DECOMP;
        else if (name.compare(std::string("count")) == 0)
                return DECAF_CONTIG_DECOMP;
        else if (name.compare(std::string("zcurve")) == 0)
                return DECAF_ZCURVE_DECOMP;
        else if (name.compare(std::string("block")) == 0)
                return DECAF_BLOCK_DECOMP;
        else if (name.compare(std::string("proc")) == 0)
                return DECAF_PROC_DECOMP;
        else if (name.compare(std::string("")) == 0)
                return DECAF_CONTIG_DECOMP;
        else
        {
                std::cerr<<"WARNING: unknown Decomposition name: "<<name<<". Using count instead."<<std::endl;
                return DECAF_CONTIG_DECOMP;
        }
}
TransportMethod
decaf::
stringToTransportMethod(std::string method)
{
    if (method.compare(std::string("mpi")) == 0)
        return DECAF_TRANSPORT_MPI;
    else if (method.compare(std::string("cci")) == 0)
        return DECAF_TRANSPORT_CCI;
    else if (method.compare(std::string("file")) == 0)
        return DECAF_TRANSPORT_FILE;
    else
    {
        std::cerr<<"WARNING: unknown Transport method "<<method<<". Using CCI instead."<<std::endl;
        return DECAF_TRANSPORT_CCI;
    }
}


Check_level
decaf::stringToCheckLevel(string check)
{
        if (!check.compare("PYTHON"))
                return CHECK_PYTHON;
        if (!check.compare("PY_AND_SOURCE"))
                return CHECK_PY_AND_SOURCE;
        if (!check.compare("EVERYWHERE"))
                return CHECK_EVERYWHERE;

        return CHECK_NONE;
}
