//---------------------------------------------------------------------------
// dataflow interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_DATAFLOW_HPP
#define DECAF_DATAFLOW_HPP

#include <map>

#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/simpleconstructdata.hpp>

#include <boost/type_index.hpp>

// transport layer specific types
#ifdef TRANSPORT_MPI
#include "transport/mpi/types.h"
#include "transport/mpi/comm.hpp"
#include "transport/mpi/redist_count_mpi.h"
#include "transport/mpi/redist_round_mpi.h"
#include "transport/mpi/redist_zcurve_mpi.h"
#include "transport/mpi/redist_block_mpi.h"
#include "transport/mpi/redist_proc_mpi.h"
#include "transport/mpi/tools.hpp"
#include <memory>
#endif

#include "types.hpp"

namespace decaf
{
    // world is partitioned into {producer, dataflow, consumer, other} in increasing rank
    class Dataflow
    {
    public:
        Dataflow(CommHandle world_comm,          // world communicator
                 DecafSizes& decaf_sizes,        // sizes of producer, dataflow, consumer
                 int prod,                       // id in workflow structure of producer node
                 int dflow,                      // id in workflow structure of dataflow link
                 int con,                        // id in workflow structure of consumer node
		         vector<pair<string,string>> list_key, // list of key/type if related to a contract
		         int check_types,					   // level of typechecking if related to a contract
                 Decomposition prod_dflow_redist // decompositon between producer and dataflow
                 = DECAF_CONTIG_DECOMP,
                 Decomposition dflow_cons_redist // decomposition between dataflow and consumer
                 = DECAF_CONTIG_DECOMP);
        ~Dataflow();
		bool put(pConstructData data, TaskType role);
		bool get(pConstructData data, TaskType role);
        DecafSizes* sizes()    { return &sizes_; }
        void shutdown();
        void err()             { ::all_err(err_); }
        // whether this rank is producer, dataflow, or consumer (may have multiple roles)
        bool is_prod()         { return((type_ & DECAF_PRODUCER_COMM) == DECAF_PRODUCER_COMM); }
        bool is_dflow()        { return((type_ & DECAF_DATAFLOW_COMM) == DECAF_DATAFLOW_COMM); }
        bool is_con()          { return((type_ & DECAF_CONSUMER_COMM) == DECAF_CONSUMER_COMM); }
        CommHandle prod_comm_handle() { return prod_comm_->handle(); }
        CommHandle con_comm_handle()  { return con_comm_->handle();  }
        Comm* prod_comm()             { return prod_comm_; }
        Comm* con_comm()              { return con_comm_;  }
        void forward();

		vector<pair<string,string>>& keys(){ return list_keys_; }
		bool is_contract(){		return bContract_;}

        // Clear the buffer of the redistribution components.
        // To call if the data model change from one iteration to another or to free memory space
        void clearBuffers(TaskType role);

        // sets a quit message into a container; caller still needs to send the message
        static
        void
        set_quit(pConstructData out_data)   // output message
		{
			shared_ptr<SimpleConstructData<int> > data  =
			    make_shared<SimpleConstructData<int> >(1);
			out_data->appendData(string("decaf_quit"), data,
			                     DECAF_NOFLAG, DECAF_SYSTEM,
			                     DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
			out_data->setSystem(true);
		}

        // tests whether a message is a quit command
        static
        bool
        test_quit(pConstructData in_data)   // input message
        {
            return in_data->hasData(string("decaf_quit"));
        }


    private:
        CommHandle world_comm_;          // handle to original world communicator
        Comm* prod_comm_;                // producer communicator
        Comm* con_comm_;                 // consumer communicator
        RedistComp* redist_prod_dflow_;  // Redistribution component between producer and dataflow
        RedistComp* redist_dflow_con_;   // Redestribution component between a dataflow and consumer
        RedistComp* redist_prod_con_;    // Redistribution component between producer and consumer
        DecafSizes sizes_;               // sizes of communicators, time steps
        int err_;                        // last error
        CommTypeDecaf type_;             // whether this instance is producer, consumer,
                                         // dataflow, or other
        int wflow_prod_id_;              // index of corresponding producer in the workflow
        int wflow_con_id_;               // index of corresponding consumer in the workflow
        int wflow_dflow_id_;             // index of corresponding link in the workflow
        bool no_link;                    // True if the Dataflow doesn't have a Link

		bool bContract_;			 // boolean to say if the dataflow has a contract or not
		int check_types_;			 // level of typechecking used; Relevant if bContract_ is set to true
		vector<pair<string,string>> list_keys_;   // keys of the data to be exchanged b/w the producer and consumer; Relevant if bContract_ is set to true
        };

} // namespace

decaf::
Dataflow::Dataflow(CommHandle world_comm,
                   DecafSizes& decaf_sizes,
                   int prod,
                   int dflow,
                   int con,
                   vector<pair<string,string>> list_keys,
                   int check_types = 0,
                   Decomposition prod_dflow_redist,
                   Decomposition dflow_cons_redist) :
    world_comm_(world_comm),
    sizes_(decaf_sizes),
    wflow_prod_id_(prod),
    wflow_dflow_id_(dflow),
    wflow_con_id_(con),
    redist_prod_dflow_(NULL),
    redist_dflow_con_(NULL),
    redist_prod_con_(NULL),
    type_(DECAF_OTHER_COMM),
    check_types_(check_types),
    no_link(false)
{
    // DEPRECATED
    // sizes is a POD struct, initialization was not allowed in C++03; used assignment workaround
    // in C++11, apparently able to initialize as above by saying sizes_(decaf_sizes)
    // sizes_.prod_size = decaf_sizes.prod_size;
    // sizes_.dflow_size = decaf_sizes.dflow_size;
    // sizes_.con_size = decaf_sizes.con_size;
    // sizes_.prod_start = decaf_sizes.prod_start;
    // sizes_.dflow_start = decaf_sizes.dflow_start;
    // sizes_.con_start = decaf_sizes.con_start;
    // sizes_.con_nsteps = decaf_sizes.con_nsteps;

    // debug, confirming that above initialization works in all compilers
    assert(sizes_.prod_size == decaf_sizes.prod_size);
    assert(sizes_.dflow_size == decaf_sizes.dflow_size);
    assert(sizes_.con_size == decaf_sizes.con_size);
    assert(sizes_.prod_start == decaf_sizes.prod_start);
    assert(sizes_.dflow_start == decaf_sizes.dflow_start);
    assert(sizes_.con_start == decaf_sizes.con_start);

    int world_rank = CommRank(world_comm); // my place in the world
    int world_size = CommSize(world_comm);

    // ensure sizes and starts fit in the world
    if (sizes_.prod_start + sizes_.prod_size > world_size   ||
        sizes_.dflow_start + sizes_.dflow_size > world_size ||
        sizes_.con_start + sizes_.con_size > world_size)
    {
        err_ = DECAF_COMM_SIZES_ERR;
        return;
	}

	// Sets the boolean value to true/false whether the dataflow is related to a contract or not
	if(list_keys.size() == 0){
		bContract_ = false;
	}
	else{
		bContract_ = true;
		list_keys_ = list_keys;
	}

    // debug
    // if (world_rank == 0)
    //     fprintf(stderr, "prod: start %d size %d dflow: start %d size %d con: start %d size %d\n",
    //             sizes_.prod_start, sizes_.prod_size, sizes_.dflow_start, sizes_.dflow_size,
    //             sizes_.con_start, sizes_.con_size);

    // communicators

    if (world_rank >= sizes_.prod_start &&                   // producer
        world_rank < sizes_.prod_start + sizes_.prod_size)
    {
        type_ |= DECAF_PRODUCER_COMM;
        prod_comm_ = new Comm(world_comm, sizes_.prod_start, sizes_.prod_start + sizes_.prod_size - 1);
    }
    if (world_rank >= sizes_.dflow_start &&                  // dataflow
        world_rank < sizes_.dflow_start + sizes_.dflow_size)
        type_ |= DECAF_DATAFLOW_COMM;
    if (world_rank >= sizes_.con_start &&                    // consumer
        world_rank < sizes_.con_start + sizes_.con_size)
    {
        type_ |= DECAF_CONSUMER_COMM;
        con_comm_ = new Comm(world_comm, sizes_.con_start, sizes_.con_start + sizes_.con_size - 1);
    }

    // producer and dataflow
    if (sizes_.dflow_size > 0 && (
        (world_rank >= sizes_.prod_start && world_rank < sizes_.prod_start + sizes_.prod_size) ||
        (world_rank >= sizes_.dflow_start && world_rank < sizes_.dflow_start + sizes_.dflow_size)))
    {
        switch(prod_dflow_redist)
        {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            // fprintf(stderr, "Using Round for prod -> dflow\n");
            redist_prod_dflow_ = new RedistRoundMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            // fprintf(stderr, "Using Count for prod -> dflow prod\n");
            redist_prod_dflow_ = new RedistCountMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            // fprintf(stderr, "Using ZCurve for prod -> dflow\n");
            redist_prod_dflow_ = new RedistZCurveMPI(sizes_.prod_start,
                                                     sizes_.prod_size,
                                                     sizes_.dflow_start,
                                                     sizes_.dflow_size,
                                                     world_comm);
            break;
        }
        case DECAF_BLOCK_DECOMP:
        {
            redist_prod_dflow_ = new RedistBlockMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }
        case DECAF_PROC_DECOMP:
        {
            redist_prod_dflow_ = new RedistProcMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   world_comm);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: policy %d unrecognized to select a redistribution component. "
                    "Using RedistCountMPI instead\n", prod_dflow_redist);
            redist_prod_dflow_ = new RedistCountMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }

        }
    }
    else
    {
        //fprintf(stderr, "No redistribution between producer and dflow needed.\n");
        redist_prod_dflow_ = NULL;
    }

    // consumer and dataflow
    if (sizes_.dflow_size > 0 && (
        (world_rank >= sizes_.dflow_start && world_rank < sizes_.dflow_start + sizes_.dflow_size) ||
        (world_rank >= sizes_.con_start && world_rank < sizes_.con_start + sizes_.con_size)))
    {
        switch(dflow_cons_redist)
        {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            // fprintf(stderr, "Using Round for dflow -> cons\n");
            redist_dflow_con_ = new RedistRoundMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            // fprintf(stderr, "Using Count for dflow -> cons\n");
            redist_dflow_con_ = new RedistCountMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            // fprintf(stderr, "Using ZCurve for dflow -> cons\n");
            redist_dflow_con_ = new RedistZCurveMPI(sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    sizes_.con_start,
                                                    sizes_.con_size,
                                                    world_comm);
            break;
        }
        case DECAF_BLOCK_DECOMP:
        {
            redist_dflow_con_ = new RedistBlockMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_PROC_DECOMP:
        {

            redist_dflow_con_ = new RedistProcMPI(sizes_.dflow_start,
                                                  sizes_.dflow_size,
                                                  sizes_.con_start,
                                                  sizes_.con_size,
                                                  world_comm);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: policy %d unrecognized to select a redistribution component. "
                    "Using RedistCountMPI instead\n", prod_dflow_redist);
            redist_dflow_con_ = new RedistCountMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }

        }
    }
    else
    {
        //fprintf(stderr, "No redistribution between dflow and cons needed.\n");
        redist_dflow_con_ = NULL;
    }

    // producer and consumer
    if (sizes_.dflow_size == 0 && (
        (world_rank >= sizes_.prod_start && world_rank < sizes_.prod_start + sizes_.prod_size) ||
        (world_rank >= sizes_.con_start && world_rank < sizes_.con_start + sizes_.con_size)))
    {
        no_link = true;

        switch(prod_dflow_redist)
        {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            // fprintf(stderr, "Using Round for dflow -> cons\n");
            redist_prod_con_ = new RedistRoundMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            // fprintf(stderr, "Using Count for dflow -> cons\n");
            redist_prod_con_ = new RedistCountMPI(sizes_.prod_start,
                                                  sizes_.prod_size,
                                                  sizes_.con_start,
                                                  sizes_.con_size,
                                                  world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            // fprintf(stderr, "Using ZCurve for dflow -> cons\n");
            redist_prod_con_ = new RedistZCurveMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_BLOCK_DECOMP:
        {
            redist_prod_con_ = new RedistBlockMPI(sizes_.prod_start,
                                                  sizes_.prod_size,
                                                  sizes_.con_start,
                                                  sizes_.con_size,
                                                  world_comm);
            break;
        }
        case DECAF_PROC_DECOMP:
        {

            redist_prod_con_ = new RedistProcMPI(sizes_.prod_start,
                                                 sizes_.prod_size,
                                                 sizes_.con_start,
                                                 sizes_.con_size,
                                                 world_comm);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: policy %d unrecognized to select a redistribution component. "
                    "Using RedistCountMPI instead\n", prod_dflow_redist);
            redist_dflow_con_ = new RedistCountMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }

        }
    }
    else
        redist_prod_con_ = NULL;

    err_ = DECAF_OK;
}

decaf::
Dataflow::~Dataflow()
{
    if (is_prod())
        delete prod_comm_;
    if (is_con())
        delete con_comm_;
    if (redist_dflow_con_)
        // TODO: Following crashes on my mac, for the hacc example, in rank 8 (dflow)
        // but not on other machines so far. Will see if it happens in other contexts,
        // or if it is just my outdated software stack -- TP
        delete redist_dflow_con_;
    if (redist_prod_dflow_)
        delete redist_prod_dflow_;
}

// puts data into the dataflow
// tags the data with the:
// - source type (producer/consumer or dataflow)
// - workflow link id corresponding to this dataflow
// - destination workflow node id or link id, depending on type of destination
// NB: source is *link* id while destination is *either node or link* id
// Returns true if ok, false if an ERROR occured
bool
decaf::
Dataflow::put(pConstructData data, TaskType role)
{
	pConstructData data_filtered;

	// If this dataflow is related to a contract, need to filter the data to be sent
	if(is_contract() && !data->isEmpty()){ //If the data received is empty, no need to check the contract
		if(data->hasData(string("decaf_quit")) ){ // To know if it's a quit message
			set_quit(data_filtered);
		}
		else{
			for(pair<string,string> pair : keys()){
				if(!data->hasData(pair.first)){// If the key is not present in the data the contract is not respected.
					fprintf(stderr, "ERROR : Contract not respected, the field \"%s\" is not present in the data model to send. Aborting.\n", pair.first.c_str());
					return false;
				}
				// Performing typechecking if needed
				if(check_types_ > 1){
					string typeName = data->getTypename(pair.first);
					if(typeName.compare(pair.second) != 0){ //The two types do not match
						fprintf(stderr, "ERROR : Contract not respected, sent type %s does not match the type %s of the field \"%s\". Aborting.\n", typeName.c_str(), pair.second.c_str(), pair.first.c_str());
						return false;
					}
				}
				data_filtered->appendData(data, pair.first);
			}
		}
	}
	else{ // No contract in this dataflow, we just send all data
		data_filtered = data;
	}

	data_filtered->removeData("src_type");
	data_filtered->removeData("link_id");
	data_filtered->removeData("dest_id");

    // encode type into message (producer/consumer or dataflow)
    shared_ptr<SimpleConstructData<TaskType> > value  =
        make_shared<SimpleConstructData<TaskType> >(role);
	data_filtered->appendData(string("src_type"), value,
                    DECAF_NOFLAG, DECAF_SYSTEM,
                    DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


    // encode dataflow link id into message
    shared_ptr<SimpleConstructData<int> > value1  =
        make_shared<SimpleConstructData<int> >(wflow_dflow_id_);
	data_filtered->appendData(string("link_id"), value1,
                     DECAF_NOFLAG, DECAF_SYSTEM,
                     DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


    if (role == DECAF_NODE)
    {
        if(no_link)
        {
            // encode destination link id into message
            shared_ptr<SimpleConstructData<int> > value2  =
                make_shared<SimpleConstructData<int> >(wflow_con_id_);
			data_filtered->appendData(string("dest_id"), value2,
                             DECAF_NOFLAG, DECAF_SYSTEM,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

            // send the message
			if(redist_prod_con_ == NULL){
                fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
				return false;
			}
			redist_prod_con_->process(data_filtered, DECAF_REDIST_SOURCE);
            redist_prod_con_->flush();
        }
        else
        {
            // encode destination link id into message
            shared_ptr<SimpleConstructData<int> > value2  =
                make_shared<SimpleConstructData<int> >(wflow_dflow_id_);
			data_filtered->appendData(string("dest_id"), value2,
                             DECAF_NOFLAG, DECAF_SYSTEM,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

            // send the message
			if(redist_prod_dflow_ == NULL){
                fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
				return false;
			}
			redist_prod_dflow_->process(data_filtered, DECAF_REDIST_SOURCE);
            redist_prod_dflow_->flush();
        }
    }
    else if (role == DECAF_LINK)
    {
        // encode destination node id into message
        shared_ptr<SimpleConstructData<int> > value2  =
            make_shared<SimpleConstructData<int> >(wflow_con_id_);
		data_filtered->appendData(string("dest_id"), value2,
                         DECAF_NOFLAG, DECAF_SYSTEM,
                         DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

        // send the message
		if(redist_dflow_con_ == NULL){
            fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
			return false;
		}
		redist_dflow_con_->process(data_filtered, DECAF_REDIST_SOURCE);
        redist_dflow_con_->flush();
	}

	data_filtered->removeData("src_type");
	data_filtered->removeData("link_id");
	data_filtered->removeData("dest_id");

	return true;
}


// gets data from the dataflow
// return true if ok, false if an ERROR occured
bool
decaf::
Dataflow::get(pConstructData data, TaskType role)
{
    if (role == DECAF_LINK)
    {
		if (redist_prod_dflow_ == NULL){
            fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
			return false;
		}
		redist_prod_dflow_->process(data, DECAF_REDIST_DEST);
        redist_prod_dflow_->flush();
    }
    else if (role == DECAF_NODE)
    {
        if(no_link)
        {
			if (redist_prod_con_ == NULL){
                fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
				return false;
			}
            redist_prod_con_->process(data, DECAF_REDIST_DEST);
            redist_prod_con_->flush();
        }
        else
        {
			if (redist_dflow_con_ == NULL){
                fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
				return false;
			}
            redist_dflow_con_->process(data, DECAF_REDIST_DEST);
            redist_dflow_con_->flush();
        }
    }

	if(test_quit(data)){
		return true;
	}

	// Checks if all keys of the contract are in the data
	if(is_contract() && !data->isEmpty()){ //If the data received is empty, no need to check the contract
		for(pair<string,string> pair : keys()){
			if(! data->hasData(pair.first)){
				fprintf(stderr, "ERROR : Contract not respected, the field \"%s\" is not received in the data model. Aborting.\n", pair.first.c_str());
				return false;
			}
			// Performing typechecking if needed
			if(check_types_ > 1){
				string typeName = data->getTypename(pair.first);
				if(typeName.compare(pair.second) != 0){ //The two types do not match
					fprintf(stderr, "ERROR : Contract not respected, received type %s does not match the type %s of the field \"%s\". Aborting.\n", typeName.c_str(), pair.second.c_str(), pair.first.c_str());
					return false;
				}
			}
		}
	}

	return true;
}

// cleanup
void
decaf::
Dataflow::shutdown()
{
    if (redist_prod_dflow_)
        redist_prod_dflow_->shutdown();
    if (redist_dflow_con_)
        redist_dflow_con_->shutdown();
    if(redist_prod_con_)
        redist_prod_con_->shutdown();
}


void
decaf::
Dataflow::clearBuffers(TaskType role)
{
    if (role == DECAF_LINK)
        redist_dflow_con_->clearBuffers();
    else if (role == DECAF_NODE && no_link)
        redist_prod_con_->clearBuffers();
    else if (role == DECAF_NODE && !no_link)
        redist_prod_dflow_->clearBuffers();
}

#endif
