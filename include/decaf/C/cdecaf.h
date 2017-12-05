#ifndef CDECAF_H
#define CDECAF_H

/* For compatibility with Clang. */
#ifndef __has_extension
#	define __has_extension(EXT) 0
#endif

#include <bredala/C/bredala.h>

#ifdef __cplusplus
#include <cstddef>
using std::size_t;
extern "C" {
#else /* __cplusplus */
#include <stdbool.h>
#include <stddef.h>
#endif /* __cplusplus */

    /* For each  type a pointer  type to an  unspecified (in fact  not existing)
     * struct is created. In fact, they are pointers to C++ objects.
     *
     * These intermediary structs are used to enforce type safety. */
    #   define DCA_DECLARE_TYPE(NAME) \
    typedef struct NAME ## _inexistent *NAME

    DCA_DECLARE_TYPE(dca_decaf);

    void
    dca_append_workflow_link(dca_decaf decaf,
                             int prod,
                             int con,
                             int start_proc,
                             int nb_proc,
                             const char* func,
                             const char* path,
                             const char* prod_dflow_redist,
                             const char* dflow_con_redist);

    void
    dca_append_workflow_node(dca_decaf decaf,
                             int start_proc,
                             int nb_procs,
                             const char* func,
                             int nb_in_links,
                             int* in_links,
                             int nb_out_links,
                             int* out_links);

    void dca_init_decaf(dca_decaf decaf);

    void dca_init_from_json(dca_decaf decaf, const char* path);

    dca_decaf
    dca_create_decaf(MPI_Comm comm_world);

    void
    dca_free_decaf(dca_decaf decaf);


    bca_constructdata*
    dca_get(dca_decaf decaf, unsigned int* nb_containers);

    void
    dca_put(dca_decaf decaf, bca_constructdata container);

    void
    dca_put_index(dca_decaf decaf, bca_constructdata container, int i);

    bool
    dca_my_node(dca_decaf decaf, const char* name);

    void
    dca_terminate(dca_decaf decaf);

    bool
    dca_has_terminated(dca_decaf decaf);

    int
    dca_get_dataflow_con_size(dca_decaf decaf, int dataflow);

    int
    dca_get_dataflow_prod_size(dca_decaf decaf, int dataflow);

    MPI_Comm
    dca_get_com(dca_decaf decaf);

    MPI_Comm
    dca_get_local_com(dca_decaf decaf);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
