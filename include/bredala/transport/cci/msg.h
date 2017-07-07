#ifndef MSG_H
#define MSG_H

#include <stdlib.h>
#include <cci.h>
#include <vector>

struct CCIchannel
{
    //char* buffer;
    std::vector<char> buffer;
    uint64_t buffer_size;
    cci_connection_t *connection;
    cci_rma_handle_t *local_rma_handle;
    cci_rma_handle_t *distant_rma_handle;

    CCIchannel()
    {
        buffer_size = 0;
        connection = nullptr;
        local_rma_handle = nullptr;
        distant_rma_handle = nullptr;
    }
};


// Type of message
enum msg_type
{
    MSG_OK = 0,
    MSG_QUIT,
    MSG_MEM_REQ,
    MSG_MEM_HANDLE,
    MSG_RMA_SENT,
    MSG_RMA_PROCESS
};


struct msg_cci
{
    msg_type type;
    int64_t it;
    union
    {
        size_t value;
        uint64_t rma_handle[4];
    };
};

#endif
