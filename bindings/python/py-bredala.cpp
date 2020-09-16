#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/vectorfield.hpp>

#include <bredala/data_model/boost_macros.h>

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

using namespace decaf;


template<typename T>
void declareSimpleField(py::module &m, std::string &typestr)
{
    using Class = SimpleField<T>;
    std::string pyclass_name = std::string("SimpleField") + typestr;
    py::class_<Class, BaseField>(m, pyclass_name.c_str())
        //.def(py::init<const T&>()) // "value"_a, "map"_a=mapConstruct(), "countable"_a=true);
        .def(py::init<T*>())
        .def("getData", &Class::getData);
}

template<typename T>
void declareVectorField(py::module &m, std::string &typestr)
{
    using Class = VectorField<T>;
    std::string pyclass_name = std::string("VectorField") + typestr;
    py::class_<Class, BaseField>(m, pyclass_name.c_str())
        .def(py::init<std::vector<T>&, int>()) //std::vector< T > &value, int element_per_items
        .def("getVector", &Class::getVector);
}

PYBIND11_MODULE(pybredala, m)
{
    m.doc() = "Bredala bindings";

    py::enum_<ConstructTypeFlag>(m, "ConstructTypeFlag")
        .value("DECAF_NOFLAG", DECAF_NOFLAG)
        .value("DECAF_NBITEM", DECAF_NBITEM)
        .value("DECAF_POS", DECAF_POS)
        .value("DECAF_MORTON", DECAF_MORTON)
        .export_values();

    py::enum_<ConstructTypeScope>(m, "ConstructTypeScope")
        .value("DECAF_SHARED", DECAF_SHARED)
        .value("DECAF_PRIVATE", DECAF_PRIVATE)
        .value("DECAF_SYSTEM", DECAF_SYSTEM)
        .export_values();

    py::enum_<ConstructTypeSplitPolicy>(m, "ConstructTypeSplitPolicy")
        .value("DECAF_SPLIT_DEFAULT", DECAF_SPLIT_DEFAULT)
        .value("DECAF_SPLIT_KEEP_VALUE", DECAF_SPLIT_KEEP_VALUE)
        .value("DECAF_SPLIT_MINUS_NBITEM", DECAF_SPLIT_MINUS_NBITEM)
        .value("DECAF_SPLIT_SEGMENTED", DECAF_SPLIT_SEGMENTED)
        .export_values();

    py::enum_<ConstructTypeMergePolicy>(m, "ConstructTypeMergePolicy")
        .value("DECAF_MERGE_DEFAULT", DECAF_MERGE_DEFAULT)
        .value("DECAF_MERGE_FIRST_VALUE", DECAF_MERGE_FIRST_VALUE)
        .value("DECAF_MERGE_ADD_VALUE", DECAF_MERGE_ADD_VALUE)
        .value("DECAF_MERGE_APPEND_VALUES", DECAF_MERGE_APPEND_VALUES)
        .value("DECAF_MERGE_BBOX_POS", DECAF_MERGE_BBOX_POS)
        .export_values();

    py::class_<BaseField>(m, "BaseField");

    py::class_<BaseData, std::shared_ptr<BaseData>>(m, "BaseData");

    //TODO: fill the template methods for other data types as well (besides getFieldDataSI)
    py::class_<ConstructData, std::shared_ptr<ConstructData>, BaseData>(m, "ConstructData")
         .def(py::init<>())
         .def("appendData", (bool (ConstructData::*)(std::string, std::shared_ptr<BaseConstructData>, ConstructTypeFlag, ConstructTypeScope, ConstructTypeSplitPolicy, ConstructTypeMergePolicy))
         &ConstructData::appendData)
         .def("appendData", (bool (ConstructData::*)(std::string, BaseField&, ConstructTypeFlag, ConstructTypeScope, ConstructTypeSplitPolicy, ConstructTypeMergePolicy))&ConstructData::appendData)
         .def("serialize", (bool (ConstructData::*) ()) &ConstructData::serialize)
         .def("isToken", &ConstructData::isToken)
         .def("setToken", &ConstructData::setToken)
         .def("getFieldDataSI", &ConstructData::getFieldData<SimpleFieldi>)
         .def("getFieldDataVF", &ConstructData::getFieldData<VectorFieldf>)
         .def("getFieldDataVC", &ConstructData::getFieldData<VectorField<char>>);

    py::class_<pConstructData>(m, "pSimple")
        .def(py::init<bool>(), "init"_a=true) //std::make_shared<ConstructData>()
        .def("get", &pConstructData::getPtr);


    //TODO: declare the other data types
    std::string type = "i";
    declareSimpleField<int>(m, type);
    declareVectorField<int>(m, type);

    std::string type_f = "f";
    declareVectorField<float>(m, type_f);

    std::string type_c = "c";
    declareVectorField<char>(m, type_c);

    std::string type2 = "uc";
    declareSimpleField<unsigned char>(m, type2);

}
