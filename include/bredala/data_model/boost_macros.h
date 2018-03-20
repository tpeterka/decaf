//---------------------------------------------------------------------------
//
// boost macros for all serializable objects
// user applications need to include this file
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------
#ifndef _BOOST_MACROS
#define _BOOST_MACROS

#include <boost/serialization/export.hpp>
#include <string>
//#include "blockconstructdata.hpp"
//#include <bredala/data_model/array3dconstructdata.hpp>


#ifdef BASE_CONSTRUCT_DATA
BOOST_CLASS_EXPORT_GUID(decaf::BaseConstructData,"BaseConstructData")
#endif

#ifdef CONSTRUCT_TYPE
BOOST_CLASS_EXPORT_GUID(decaf::ConstructData,"ConstructData")
#endif

#ifdef VECTOR_CONSTRUCT_DATA
BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<float>,"VectorConstructData<float>")
BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<int>,"VectorConstructData<int>")
BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<double>,"VectorConstructData<double>")
BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<unsigned int>,"VectorConstructData<unsigned int")
BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<char>,"VectorConstructData<char>")
#endif

#ifdef SIMPLE_CONSTRUCT_DATA
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<float>,"SimpleConstructData<float>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<int>,"SimpleConstructData<int>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<double>,"SimpleConstructData<double>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<unsigned int>,"SimpleConstructData<unsigned int>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<char>,"SimpleConstructData<char>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<unsigned char>,
                        "SimpleConstructData<unsigned char>")
//BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<std::string>,
//                        "SimpleConstructData<std::string>")
#endif

#ifdef ARRAY_CONSTRUCT_DATA
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<float>,"ArrayConstructData<float>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<int>,"ArrayConstructData<int>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<unsigned int>,"ArrayConstructData<unsigned int>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<double>,"ArrayConstructData<double>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<char>,"ArrayConstructData<char>")
#endif

#ifdef BLOCK_CONSTRUCT_DATA
BOOST_CLASS_EXPORT_GUID(decaf::BlockConstructData,"BlockConstructData")
#endif

#ifdef DECAF_BLOCK_HPP
BOOST_CLASS_EXPORT_GUID(decaf::Block<1>,"decaf::Block<1>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<2>,"decaf::Block<2>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<3>,"decaf::Block<3>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<4>,"decaf::Block<4>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<5>,"decaf::Block<5>")
#endif

#ifdef ARRAY_3D_CONSTRUCT_DATA
BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<float>,"ArrayNDimConstructData<float3>")
BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<int>,"ArrayNDimConstructData<int3>")
BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<double>,"ArrayNDimConstructData<double3>")
#endif

#endif
