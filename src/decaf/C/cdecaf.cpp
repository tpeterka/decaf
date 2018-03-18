#include <decaf/C/cdecaf.h>

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
#include <decaf/decaf.hpp>

namespace {

using namespace std;
using namespace decaf;

    struct Base
    {
            virtual ~Base()
            {}
    };


    class DecafHolder : public Base
    {
    public:
        DecafHolder(MPI_Comm communicator)
        {
            communicator_ = communicator;
            decaf_ = NULL;
            hasReceivedTerminate_ = false;
        }

        ~DecafHolder()
        {
            messages_.clear();
            if(decaf_) delete decaf_;
        }

        void appendLink(int prod,
                        int con,
                        int start_proc,
                        int nb_prob,
                        const char* func,
                        const char* path,
                        const char* prod_dflow_redist,
                        const char* dflow_con_redist)
        {
            WorkflowLink link;
            link.prod = prod;
            link.con = con;
            link.start_proc = start_proc;
            link.nprocs = nb_prob;
            link.func = string(func);
            link.path = string(path);
            link.prod_dflow_redist = string(prod_dflow_redist);
            link.dflow_con_redist = string(dflow_con_redist);

            workflow_.links.push_back(link);
        }

        void appendNode(int start_proc,
                        int nb_procs,
                        const char* func,
                        int nb_in_links,
                        int* in_links,
                        int nb_out_links,
                        int* out_links)
        {
            WorkflowNode node;
            node.start_proc = start_proc;
            node.nprocs = nb_procs;
            node.func = string(func);
            for(int i = 0; i < nb_in_links; i++)
                node.in_links.push_back(in_links[i]);
            for(int i = 0; i < nb_out_links; i++)
                node.out_links.push_back(out_links[i]);

            workflow_.nodes.push_back(node);
        }

        void initDecaf()
        {
            decaf_ = new Decaf( communicator_, workflow_ );
        }

        Decaf* getDecaf(){ return decaf_; }

        bool get()
        {
            if(!decaf_->get(messages_))
            {
                // Has to check if get return 0 because all the ports
                // have received a terminate message or if it is because there is
                // no input ports
                // If no input ports, we don't receive terminate
                if(messages_.size() > 0)
                    hasReceivedTerminate_ = true;
                return false;
            }

            return true;
        }

        unsigned int getNbMessages(){ return messages_.size(); }
        pConstructData* getMessages(){ return &messages_[0]; }

        bool getTerminated(){ return hasReceivedTerminate_; }

        void initFromJSON(const char* path)
        {
            Workflow::make_wflow_from_json(workflow_, path);
            decaf_ = new Decaf( communicator_, workflow_ );
        }

    private:
        Workflow workflow_;
        Decaf* decaf_;
        MPI_Comm communicator_;
        vector<pConstructData> messages_;
        bool hasReceivedTerminate_;
    };

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
    BOX_TRAITS(dca_decaf, DecafHolder);
    BOX_TRAITS(bca_constructdata, pConstructData);

    /* For derivated classes we only associate them with the boxes of their base
     * class. */
    // No derived class

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

    void
    dca_append_workflow_link(dca_decaf decaf,
                             int prod,
                             int con,
                             int start_proc,
                             int nb_proc,
                             const char* func,
                             const char* path,
                             const char* prod_dflow_redist,
                             const char* dflow_con_redist)
    {
        unbox(decaf)->appendLink(prod, con, start_proc,
                                 nb_proc,  func, path,
                                 prod_dflow_redist,
                                 dflow_con_redist);
    }

    void
    dca_append_workflow_node(dca_decaf decaf,
                             int start_proc,
                             int nb_procs,
                             const char* func,
                             int nb_in_links,
                             int* in_links,
                             int nb_out_links,
                             int* out_links)
    {
        unbox(decaf)->appendNode(start_proc, nb_procs, func,
                                 nb_in_links, in_links,
                                 nb_out_links, out_links);
    }

    void dca_init_decaf(dca_decaf decaf)
    {
        unbox(decaf)->initDecaf();
    }

    void dca_init_from_json(dca_decaf decaf, const char* path)
    {
        unbox(decaf)->initFromJSON(path);
    }

    dca_decaf
    dca_create_decaf(MPI_Comm comm_world)
    {
        return box(new DecafHolder(comm_world));
    }

    void
    dca_free_decaf(dca_decaf decaf)
    {
        delete unbox(decaf);
    }

    bca_constructdata*
    dca_get(dca_decaf decaf, unsigned int* nb_containers)
    {
        DecafHolder* handler = unbox(decaf);
        if(!handler->get())
            return NULL;


        unsigned int nbMessages = handler->getNbMessages();
        pConstructData* messages = handler->getMessages();

        bca_constructdata* containers = (bca_constructdata*)(malloc(nbMessages*sizeof(bca_constructdata)));
        for(unsigned int i = 0; i < nbMessages; i++)
            containers[i] = box(new pConstructData(messages[i].getPtr()));

        *nb_containers = nbMessages;
        return containers;
    }

    void
    dca_put(dca_decaf decaf, bca_constructdata container)
    {
        DecafHolder* handler = unbox(decaf);
        pConstructData* message = unbox(container);

        handler->getDecaf()->put(*message);
    }

    void
    dca_put_index(dca_decaf decaf, bca_constructdata container, int i)
    {
        DecafHolder* handler = unbox(decaf);
        pConstructData* message = unbox(container);

        handler->getDecaf()->put(*message, i);
    }

    bool
    dca_my_node(dca_decaf decaf, const char* name)
    {
        return unbox(decaf)->getDecaf()->my_node(name);
    }

    bool
    dca_has_terminated(dca_decaf decaf)
    {
        return unbox(decaf)->getTerminated();
    }

    void
    dca_terminate(dca_decaf decaf)
    {
        return unbox(decaf)->getDecaf()->terminate();
    }

    int
    dca_get_dataflow_con_size(dca_decaf decaf, int dataflow)
    {
        DecafHolder* handler = unbox(decaf);
        return handler->getDecaf()->dataflow(dataflow)->sizes()->con_size;
    }

    int
    dca_get_dataflow_prod_size(dca_decaf decaf, int dataflow)
    {
        DecafHolder* handler = unbox(decaf);
        return handler->getDecaf()->dataflow(dataflow)->sizes()->prod_size;
    }

    MPI_Comm
    dca_get_com(dca_decaf decaf)
    {
        DecafHolder* handler = unbox(decaf);
        return handler->getDecaf()->prod_comm_handle();
    }

    MPI_Comm
    dca_get_local_com(dca_decaf decaf)
    {
        DecafHolder* handler = unbox(decaf);
        return handler->getDecaf()->local_comm_handle();
    }
}
