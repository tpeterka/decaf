#include <decaf/workflow.hpp>

using namespace decaf;
void
WorkflowNode::add_out_link(int link)
{
    out_links.push_back(link);
}

void
WorkflowNode::
add_in_link(int link)
{
    in_links.push_back(link);
}

bool
Workflow::my_node(int proc, int node)         // whether my process is part of this node
{
    return(proc >= nodes[node].start_proc &&
           proc <  nodes[node].start_proc + nodes[node].nprocs);
}

bool
Workflow::my_link(int proc, int link)         // whether my process is part of this link
{
    return(proc >=links[link].start_proc &&
           proc < links[link].start_proc + links[link].nprocs);
}

bool
Workflow::my_in_link(int proc, int link)      // whether my process gets input data from this link
{
    for (size_t i = 0; i< nodes.size(); i++)
    {
        if (proc >= nodes[i].start_proc && // proc is mine
                proc <  nodes[i].start_proc + nodes[i].nprocs)
        {
            for (size_t j = 0; j < nodes[i].in_links.size(); j++)
            {
                if (nodes[i].in_links[j] == link)
                    return true;
            }
        }
    }
    return false;
}

bool
Workflow::my_out_link(int proc, int link)      // whether my process puts output data to this link
{
    for (size_t i = 0; i< nodes.size(); i++)
    {
        if (proc >= nodes[i].start_proc && // proc is mine
                proc <  nodes[i].start_proc + nodes[i].nprocs)
        {
            for (size_t j = 0; j < nodes[i].out_links.size(); j++)
            {
                if (nodes[i].out_links[j] == link)
                    return true;
            }
        }
    }
    return false;
}


void
Workflow::make_wflow_from_json( Workflow& workflow, const string& json_path )
{
    string json_filename = json_path;
    if (json_filename.length() == 0)
    {
        fprintf(stderr, "No name filename provided for the JSON file. Falling back on the DECAF_JSON environment variable\n");
        const char* env_path = std::getenv("DECAF_JSON");
        if (env_path == NULL)
        {
            fprintf(stderr, "ERROR: The environment variable DECAF_JSON is not defined. Unable to find the workflow graph definition.\n");
            exit(1);
        }
        json_filename = string(env_path);
    }

    try {

        bpt::ptree root;

        /*
       * Use Boost::property_tree to read/parse the JSON file. This
       * creates a property_tree object which we'll then query to get
       * the information we want.
       *
       * N.B. Unlike what is provided by most JSON/XML parsers, the
       * property_tree is NOT a DOM tree, although processing it is somewhat
       * similar to what you'd do with a DOM tree. Refer to the Boost documentation
       * for more information.
       */

        bpt::read_json( json_filename, root );

        /*
        * iterate over the list of nodes, creating and populating WorkflowNodes as we go
        */
        for ( auto &&v : root.get_child( "workflow.nodes" ) )
        {
            WorkflowNode node;
            node.out_links.clear();
            node.in_links.clear();
            node.inports.clear();
            node.outports.clear();
            /* we defer actually linking nodes until we read the edge list */

            node.start_proc = v.second.get<int>("start_proc");
            node.nprocs = v.second.get<int>("nprocs");
            node.func = v.second.get<string>("func");

            // Retrieving the input ports, if present
            boost::optional<bpt::ptree&> pt_inputs = v.second.get_child_optional("inports");
            if (pt_inputs)
            {
                for (auto& item : pt_inputs->get_child(""))
                    node.inports.push_back(item.second.get_value<string>());
            }

            // Retrieving the output ports, if present
            boost::optional<bpt::ptree&> pt_outputs = v.second.get_child_optional("outports");
            if (pt_outputs)
            {
                for (auto& item : pt_outputs->get_child(""))
                    node.outports.push_back(item.second.get_value<string>());
            }

            workflow.nodes.push_back( node );
        } // End for workflow.nodes

        string sCheck = root.get<string>("workflow.filter_level");
        Check_level check_level = stringToCheckLevel(sCheck);

        /*
        * similarly for the edges
        */
        for ( auto &&v : root.get_child( "workflow.edges" ) )
        {
            WorkflowLink link;

            /* link the nodes correctly(?) */
            link.prod = v.second.get<int>("source");
            link.con = v.second.get<int>("target");

            workflow.nodes.at( link.prod ).out_links.push_back( workflow.links.size() );
            workflow.nodes.at( link.con ).in_links.push_back( workflow.links.size() );

            link.start_proc = v.second.get<int>("start_proc");
            link.nprocs = v.second.get<int>("nprocs");
            link.prod_dflow_redist = v.second.get<string>("prod_dflow_redist");
            link.check_level = check_level;
            link.name = v.second.get<string>("name");
            link.transport_method = v.second.get<string>("transport");
            boost::optional<int> opt_tokens = v.second.get_optional<int>("tokens");
            if(opt_tokens)
                link.tokens = opt_tokens.get();
            else
                link.tokens = 0;

            if(link.nprocs != 0){ // Only used if there are procs on this link
                link.path = v.second.get<string>("path");
                link.func = v.second.get<string>("func");
                link.dflow_con_redist = v.second.get<string>("dflow_con_redist");
            }

            // Default values when no contract nor contract link are involved
            link.bAny = false;
            link.list_keys.clear();
            link.keys_link.clear();

            // Retrieving the name of source and target ports
            boost::optional<string> srcP = v.second.get_optional<string>("sourcePort");
            boost::optional<string> destP = v.second.get_optional<string>("targetPort");
            if (srcP && destP)
            {
                // If there are ports, retrieve the port names and then check if there are contracts associated
                link.srcPort = srcP.get();
                link.destPort = destP.get();

                // Retrieving the contract, if present
                boost::optional<bpt::ptree&> pt_keys = v.second.get_child_optional("keys");
                if (pt_keys){
                    for(bpt::ptree::value_type &value: pt_keys.get()){
                        ContractKey field;
                        field.name = value.first;

                        // Didn't find a nicer way of doing this...
                        auto i = value.second.begin();
                        field.type = i->second.get<string>("");
                        i++;
                        field.period = i->second.get<int>("");
                        //////

                        link.list_keys.push_back(field);
                    }
                }
                // Retrieving the contract on the link, if present
                boost::optional<bpt::ptree&> pt_keys_link = v.second.get_child_optional("keys_link");
                if (pt_keys_link){
                    for(bpt::ptree::value_type &value: pt_keys_link.get()){
                        ContractKey field;
                        field.name = value.first;

                        // Didn't find a nicer way of doing this...
                        auto i = value.second.begin();
                        field.type = i->second.get<string>("");
                        i++;
                        field.period = i->second.get<int>("");
                        //////

                        link.keys_link.push_back(field);
                    }
                }
                boost::optional<bool> pt_any = v.second.get_optional<bool>("bAny");
                if (pt_any){
                    link.bAny = pt_any.get();
                }

            }

            // Retrieving information on streams and buffers
            boost::optional<string> opt_stream = v.second.get_optional<string>("stream");
            if (opt_stream)
            {
                link.manala_info.stream = opt_stream.get();
                if(link.manala_info.stream != "none")
                {
                    boost::optional<string> opt_frame_policy = v.second.get_optional<std::string>("frame_policy");
                    if (opt_frame_policy)
                        link.manala_info.frame_policy = opt_frame_policy.get();
                    else
                        link.manala_info.frame_policy = "none";
                    boost::optional<unsigned int> opt_prod_output = v.second.get_optional<unsigned int>("prod_output_freq");
                    if (opt_prod_output)
                        link.manala_info.prod_freq_output = opt_prod_output.get();
                    else
                        link.manala_info.prod_freq_output = 1;
                    boost::optional<unsigned int> opt_low_output = v.second.get_optional<unsigned int>("low_output_freq");
                    if (opt_low_output)
                        link.manala_info.low_frequency = opt_low_output.get();
                    else
                        link.manala_info.low_frequency = 0;
                    boost::optional<unsigned int> opt_high_output = v.second.get_optional<unsigned int>("high_output_freq");
                    if (opt_high_output)
                        link.manala_info.high_frequency = opt_high_output.get();
                    else
                        link.manala_info.high_frequency = 0;
                    boost::optional<string> opt_storage_policy = v.second.get_optional<std::string>("storage_collection_policy");
                    if (opt_storage_policy)
                        link.manala_info.storage_policy = opt_storage_policy.get();
                    else
                        link.manala_info.storage_policy = "greedy";


                    // TODO CHECK if this is possible even when there are no "strorage_types" in the tree
                    // TODO is it better to use get_optional? What does v.second.count do?
                    if (v.second.count("storage_types") > 0)
                    {
                        for (auto &types : v.second.get_child("storage_types"))
                        {
                            StorageType type = stringToStoragePolicy(types.second.data());
                            link.manala_info.storages.push_back(type);
                        }
                    }

                    if (v.second.count("max_storage_sizes") > 0)
                    {
                        for (auto &max_size : v.second.get_child("max_storage_sizes"))
                        {
                            link.manala_info.storage_max_buffer.push_back(max_size.second.get_value<unsigned int>());
                        }
                    }

                    // Checking that the storage is properly setup
                    if (link.manala_info.storages.size() != link.manala_info.storage_max_buffer.size())
                    {
                        fprintf(stderr, "ERROR: the number of storage layer does not match the number of storage max sizes.\n");
                        exit(1);
                    }
                    if (link.manala_info.storages.empty())
                    {
                        fprintf(stderr, "ERROR: using a stream with buffering capabilities but no storage layers given. Declare at least one storage layer.\n");
                        exit(1);
                    }
                }

            }
            else
                link.manala_info.stream = "none";

            workflow.links.push_back( link );
        } // End for workflow.links
    }
    catch( const bpt::json_parser_error& jpe )
    {
        cerr << "JSON parser exception: " << jpe.what() << endl;
        exit(1);
    }
    catch ( const bpt::ptree_error& pte )
    {
        cerr << "property_tree exception: " << pte.what() << endl;
        exit(1);
    }

}
