#include <decaf/decaf.hpp>

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <cstdlib>
#include <assert.h>
#include <math.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
namespace py = pybind11;

using namespace pybind11::literals;

#include <mpi.h>

using namespace decaf;
using namespace std;

PYBIND11_MODULE(pydecaf, m)
{

    m.doc() = "Decaf bindings";

    py::class_<Workflow>(m, "Workflow")
        .def(py::init<>())
        .def_static("makeWflow", &Workflow::make_wflow_from_json);

    //TODO: add other variants of put/get if necessary.
    py::class_<Decaf>(m, "Decaf")
        .def(py::init([](long comm_, Workflow& w)
                         {
                             MPI_Comm comm = *static_cast<MPI_Comm*>(reinterpret_cast<void*>(comm_));
                             return new Decaf(comm,w);
                         }))
        .def("put", (bool (Decaf::*)(pConstructData)) &Decaf::put, "Put a message on all outgoing dataflows")
        .def("put", (bool (Decaf::*)(pConstructData, std::string)) &Decaf::put, "Put a message on a specific dataflow")
        .def("get", (bool (Decaf::*)(pConstructData, std::string)) &Decaf::get, "Get a message from a specific dataflow")
        .def("local_comm_rank", &Decaf::local_comm_rank)
        .def("workflow_comm_rank", &Decaf::workflow_comm_rank)
        .def("terminate", &Decaf::terminate);
}
