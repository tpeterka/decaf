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
#include <string>

#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simpleconstructdata.hpp>
#include <bredala/data_model/msgtool.hpp>

#include <boost/type_index.hpp>

// transport layer specific types
#ifdef TRANSPORT_MPI
#include <bredala/transport/mpi/types.h>
#include <bredala/transport/mpi/comm.h>
#include <bredala/transport/mpi/redist_count_mpi.h>
#include <bredala/transport/mpi/redist_round_mpi.h>
#include <bredala/transport/mpi/redist_zcurve_mpi.h>
#include <bredala/transport/mpi/redist_block_mpi.h>
#include <bredala/transport/mpi/redist_proc_mpi.h>
#include <bredala/transport/mpi/comm.h>
#include <bredala/transport/mpi/channel.hpp>
#include <manala/datastream/datastreamdoublefeedback.hpp>
#include <manala/datastream/datastreamsinglefeedback.hpp>
#include <manala/datastream/datastreamsinglenolink.hpp>
#endif

#ifdef TRANSPORT_CCI
#include <bredala/transport/cci/redist_block_cci.h>
#include <bredala/transport/cci/redist_count_cci.h>
#include <bredala/transport/cci/redist_proc_cci.h>
#include <bredala/transport/cci/redist_round_cci.h>
#endif

#ifdef TRANSPORT_FILE
#include <bredala/transport/file/redist_block_file.h>
#include <bredala/transport/file/redist_count_file.h>
#include <bredala/transport/file/redist_proc_file.h>
#include <bredala/transport/file/redist_round_file.h>
#endif

#include <decaf/types.hpp>
#include <decaf/workflow.hpp>
#include <memory>
#include <queue>


namespace decaf
{
// world is partitioned into {producer, dataflow, consumer, other} in increasing rank
class Dataflow
{
public:
    Dataflow(CommHandle world_comm,             ///<  world communicator
             int workflow_size,                 ///<  size of the workflow
             int workflow_rank,                 ///<  rank in the workflow
             DecafSizes& decaf_sizes,           ///<  sizes of producer, dataflow, consumer
             int prod,                          ///<  id in workflow structure of producer node
             int dflow,                         ///<  id in workflow structure of dataflow link
             int con,                           ///<  id in workflow structure of consumer node
             WorkflowLink wflowLink,
             Decomposition prod_dflow_redist,   ///<  decompositon between producer and dataflow
	     //! decomposition between dataflow and consumer
             Decomposition dflow_cons_redist);

    ~Dataflow();
    //! passes the data to the corresponding redistribution component that manipulates and transmits the data to its destination.
    bool put(pConstructData& data, TaskType role);
    //! receives the data on the corresponding redistribution component and transmits it to the task.
    bool get(pConstructData& data, TaskType role);

    void signalReady(); ///< deprecated (not used)

    pConstructData& filterPut(pConstructData& data, pConstructData& data_filtered, TaskType& role, bool& data_changed, bool& filtered_empty);
    void filterGet(pConstructData& data, TaskType role);

    DecafSizes* sizes();
    void shutdown();
    void err();
    // whether this rank is producer, dataflow, or consumer (may have multiple roles)
    bool is_prod();
    bool is_dflow();
    bool is_con();
    bool is_prod_root();
    bool is_dflow_root();
    bool is_con_root();
    CommHandle prod_comm_handle();
    CommHandle con_comm_handle();
    CommHandle dflow_comm_handle();
    Comm* prod_comm();
    Comm* dflow_comm();
    Comm* con_comm();
    void forward(); ///< deprecated (not used)

    vector<ContractKey>& keys();
    bool has_contract();

    vector<ContractKey>& keys_link();
    bool has_contract_link();
    bool isAny();

    string srcPort();
    string destPort();

    bool isClose();
    void setClose();

    // Clear the buffer of the redistribution components.
    // To call if the data model change from one iteration to another or to free memory space
    void clearBuffers(TaskType role);

private:
    CommHandle world_comm_;          // handle to original world communicator
    Comm* prod_comm_;                // producer communicator
    Comm* con_comm_;                 // consumer communicator
    Comm* dflow_comm_;               // dflow communicator
    int world_rank_;
    int world_size_;
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
    bool no_link_;                   // True if the Dataflow doesn't have a Link
    bool use_stream_;                // True if the Dataflow manages a buffer.

    string srcPort_;                 // Portname of the source
    string destPort_;                // Portname of the destination
    int tokens_;                     // Number of empty message to receive on destPort_
    bool bClose_;                    // Determines if a quit message has been received when calling a get

    // Buffer infos
    StreamPolicy stream_policy_;     // Type of stream to use
    Datastream* stream_;

    // Used for filtering/checking of contracts
    int iteration;                   // Iterations counter
    bool prod_link_overlap;          // Whether there is overlaping between producer and link

    bool bContract_;                 // boolean to say if the dataflow has a contract or not
    bool bContractLink_;             // boolean to say if there is a contract on the link of the dataflow
    bool bAny_;                      // Whether the filtering will check the contracts but keep any other field or not
    Check_level check_level_;   	 // level of typechecking used; Relevant if bContract_ is set to true
    vector<ContractKey> keys_;       // keys of the data to be exchanged b/w the producer and consumer; Relevant if bContract_ is set to true
    //									b/w the producer and link if bContractLink_ is set to true
    vector<ContractKey> keys_link_;	// keys of the data to be exchanged b/w the link and consumer, is bContractLink_ is set to true
};// End of class Dataflow

} // namespace



#endif
