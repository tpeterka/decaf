if(${transport_mpi})
    add_subdirectory(mpi)
endif(${transport_mpi})

if(${transport_cci})
    add_subdirectory(cci)
endif(${transport_cci})
