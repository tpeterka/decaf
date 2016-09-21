//---------------------------------------------------------------------------
//
// diy block serialization (tess data structure)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------
#ifndef _BLOCK_SERIALIZATION
#define _BLOCK_SERIALIZATION

#include <decaf/data_model/baseconstructdata.hpp>
#include <boost/serialization/array.hpp>
#include "tess/tess.h"
#include "tess/tess.hpp"

// using namespace decaf;
using namespace std;

BOOST_SERIALIZATION_SPLIT_FREE(dblock_t)
BOOST_IS_BITWISE_SERIALIZABLE(bb_c_t)
BOOST_IS_BITWISE_SERIALIZABLE(tet_t)
BOOST_IS_BITWISE_SERIALIZABLE(gb_t)

namespace boost
{
    namespace serialization
    {
        template<class Archive>
        void save(Archive& ar, const dblock_t& b, unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(b.gid);
            ar & BOOST_SERIALIZATION_NVP(b.mins);
            ar & BOOST_SERIALIZATION_NVP(b.maxs);
            // TODO: get the following 2 lines to compile
            // ar & BOOST_SERIALIZATION_NVP(b.box);
            // ar & BOOST_SERIALIZATION_NVP(b.data_bounds);
            ar & BOOST_SERIALIZATION_NVP(b.num_orig_particles);
            ar & BOOST_SERIALIZATION_NVP(b.num_particles);
            ar & boost::serialization::make_array<float>(b.particles, 3 * b.num_particles);
            ar & boost::serialization::make_array<int>(b.rem_gids,
                                                       b.num_particles - b.num_orig_particles);
            ar & boost::serialization::make_array<int>(b.rem_lids,
                                                       b.num_particles - b.num_orig_particles);
            ar & BOOST_SERIALIZATION_NVP(b.num_grid_pts);
            ar & boost::serialization::make_array<float>(b.density, b.num_grid_pts);
            ar & BOOST_SERIALIZATION_NVP(b.complete);
            ar & BOOST_SERIALIZATION_NVP(b.num_tets);
            ar & boost::serialization::make_array<tet_t>(b.tets, b.num_tets);
            ar & boost::serialization::make_array<int>(b.vert_to_tet, b.num_particles);

            // TODO: append diy link, need master and lid
            // diy::Link* link = master.link(0);             // TODO, only block 0 for now
            // vector<diy::BlockID> neighbors_;
            // for (size_t i = 0; i < link->size(); i++)
            //     neighbors_.push_back(link->target(i));
            // VectorField<diy::BlockID> neighbors(neighbors_, 1);
        }
        template<class Archive>
        void load(Archive& ar, dblock_t& b, unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(b.gid);
            ar & BOOST_SERIALIZATION_NVP(b.mins);
            ar & BOOST_SERIALIZATION_NVP(b.maxs);
            // TODO: get the following 2 lines to compile
            // ar & BOOST_SERIALIZATION_NVP(b.box);
            // ar & BOOST_SERIALIZATION_NVP(b.data_bounds);
            ar & BOOST_SERIALIZATION_NVP(b.num_orig_particles);
            ar & BOOST_SERIALIZATION_NVP(b.num_particles);
            b.particles = new float[3 * b.num_particles];
            ar & boost::serialization::make_array<float>(b.particles, 3 * b.num_particles);
            b.rem_gids = new int[b.num_particles - b.num_orig_particles];
            ar & boost::serialization::make_array<int>(b.rem_gids,
                                                       b.num_particles - b.num_orig_particles);
            b.rem_lids = new int[b.num_particles - b.num_orig_particles];
            ar & boost::serialization::make_array<int>(b.rem_lids,
                                                       b.num_particles - b.num_orig_particles);
            ar & BOOST_SERIALIZATION_NVP(b.num_grid_pts);
            b.density = new float[b.num_grid_pts];
            ar & boost::serialization::make_array<float>(b.density, b.num_grid_pts);
            ar & BOOST_SERIALIZATION_NVP(b.complete);
            ar & BOOST_SERIALIZATION_NVP(b.num_tets);
            b.tets = new tet_t[b.num_tets];
            ar & boost::serialization::make_array<tet_t>(b.tets, b.num_tets);
            b.vert_to_tet = new int[b.num_particles];
            ar & boost::serialization::make_array<int>(b.vert_to_tet, b.num_particles);

            // append diy link, need master and lid
            // diy::Link* link = master.link(0);             // TODO, only block 0 for now
            // vector<diy::BlockID> neighbors_;
            // for (size_t i = 0; i < link->size(); i++)
            //     neighbors_.push_back(link->target(i));
            // VectorField<diy::BlockID> neighbors(neighbors_, 1);
        }
    }
}

BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<dblock_t>, "ArrayConstructData<dblock_t>")

#endif
