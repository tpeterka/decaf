//---------------------------------------------------------------------------
//
// Example of serializations with Boost and manipulation with the data model
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <bredala/data_model/simpleconstructdata.hpp>
#include <bredala/data_model/vectorconstructdata.hpp>
#include <bredala/data_model/constructtype.h>
#include "bboxconstructData.hpp"
#include <bredala/data_model/arrayconstructdata.hpp>
#include "tetcounterdata.hpp"
#include "arraytetsdata.hpp"
#include "verttotetdata.hpp"

#include "tess/tess.h"
#include "tess/tess.hpp"
#include "tess/tet.h"
#include "tess/tet-neighbors.h"

#include <diy/master.hpp>
#include <diy/io/block.hpp>

using namespace decaf;
using namespace std;

void containerToBlock(std::shared_ptr<ConstructData> container, dblock_t* block)
{

    std::shared_ptr<BaseConstructData> field = container->getData("gid");
    if(field)
    {
        std::shared_ptr<SimpleConstructData<int> > gid = dynamic_pointer_cast<SimpleConstructData<int> >(field);
        block->gid = gid->getData();
    }
    else
        std::cerr<<"ERROR : gid not found in the map."<<std::endl;

    field = container->getData("num_orig_particles");
    if(field)
    {
        std::shared_ptr<SimpleConstructData<int> > numPart = dynamic_pointer_cast<SimpleConstructData<int> >(field);
        block->num_orig_particles = numPart->getData();
    }
    else
        std::cerr<<"ERROR : num_orig_particles not found in the map"<<std::endl;

    field = container->getData("num_particles");
    if(field)
    {
        std::shared_ptr<SimpleConstructData<int> > numPart = dynamic_pointer_cast<SimpleConstructData<int> >(field);
        block->num_particles = numPart->getData();
    }
    else
        std::cerr<<"ERROR : num_particles not found in the map"<<std::endl;

    field = container->getData("bbox");
    if(field)
    {
        std::shared_ptr<BBoxConstructData > bbox = dynamic_pointer_cast<BBoxConstructData>(field);
        std::vector<float> &b = bbox->getVector();
        block->mins[0] = b[0];
        block->mins[1] = b[1];
        block->mins[2] = b[2];
        block->maxs[0] = b[3];
        block->maxs[1] = b[4];
        block->maxs[2] = b[5];
    }
    else
        std::cerr<<"ERROR : bbox not found in the map"<<std::endl;

    field = container->getData("pos");
    if(field)
    {
        std::shared_ptr<ArrayConstructData<float> > particles = dynamic_pointer_cast<ArrayConstructData<float> >(field);
        block->particles = particles->getArray();
    }
    else
        std::cerr<<"ERROR : pos not found in the map"<<std::endl;

    field = container->getData("num_tets");
    if(field)
    {
        std::shared_ptr<SimpleConstructData<int> > numTets = dynamic_pointer_cast<SimpleConstructData<int> >(field);
        block->num_tets = numTets->getData();
    }
    else
        std::cerr<<"ERROR : num_tets not found in the map"<<std::endl;

    field = container->getData("tets");
    if(field)
    {
        std::shared_ptr<ArrayTetsData > tets = dynamic_pointer_cast<ArrayTetsData>(field);
        block->tets = tets->getArray();
    }
    else
        std::cerr<<"ERROR : tets not found in the map"<<std::endl;

    field = container->getData("verttotet");
    if(field)
    {
        std::shared_ptr<VertToTetData> table = dynamic_pointer_cast<VertToTetData>(field);
        block->vert_to_tet = table->getArray();
    }
    else
        std::cerr<<"ERROR : verttotet not found in the map"<<std::endl;

    //Initializing the array which we don't merged
    block->rem_gids = new int[block->num_particles - block->num_orig_particles];
    block->rem_lids = new int[block->num_particles - block->num_orig_particles];
    block->num_grid_pts = 0;
    block->density = new float[1];
}

void printBlock(dblock_t* block)
{
    std::cout<<"Gid : "<<block->gid<<std::endl;
    std::cout<<"BBox ["<<block->mins[0]<<","<<block->mins[1]<<","<<block->mins[2]<<"]"<<std::endl;
    std::cout<<"     ["<<block->maxs[0]<<","<<block->maxs[1]<<","<<block->maxs[2]<<"]"<<std::endl;
    std::cout<<"Number of particules : "<<block->num_particles<<std::endl;
    if(block->num_particles > 0)
        std::cout<<"First particles : ["<<block->particles[0]<<","<<block->particles[1]<<","<<block->particles[2]<<"]"<<std::endl;
    std::cout<<"Number of tets : "<<block->num_tets<<std::endl;
    if(block->num_tets > 0)
    {
        std::cout<<"First tets : ["<<block->tets[0].verts[0]<<","<<block->tets[0].verts[1]<<","<<block->tets[0].verts[2]<<","<<block->tets[0].verts[3]<<"]"<<std::endl;
        std::cout<<"           : ["<<block->tets[0].tets[0]<<","<<block->tets[0].tets[1]<<","<<block->tets[0].tets[2]<<","<<block->tets[0].tets[3]<<"]"<<std::endl;
        std::cout<<"Last tets : ["<<block->tets[block->num_tets-1].verts[0]<<","<<block->tets[block->num_tets-1].verts[1]<<","<<block->tets[block->num_tets-1].verts[2]<<","<<block->tets[block->num_tets-1].verts[3]<<"]"<<std::endl;
        std::cout<<"           : ["<<block->tets[block->num_tets-1].tets[0]<<","<<block->tets[block->num_tets-1].tets[1]<<","<<block->tets[block->num_tets-1].tets[2]<<","<<block->tets[block->num_tets-1].tets[3]<<"]"<<std::endl;
    }
}

void countNbTetsContainer(std::shared_ptr<ConstructData> container)
{
    std::shared_ptr<BaseConstructData> field = container->getData("tets");
    std::shared_ptr<BaseConstructData> field2 = container->getData("pos");
    if(field && field2)
    {
        std::shared_ptr<ArrayTetsData > tets = dynamic_pointer_cast<ArrayTetsData>(field);
        std::shared_ptr<ArrayConstructData<float> > pos = dynamic_pointer_cast<ArrayConstructData<float> >(field2);
        tet_t* array = tets->getArray();
        float* particles = pos->getArray();
        std::cout<<"Number of particles : "<<pos->getNbItems()<<std::endl;
        std::cout<<"Size of the array of particles : "<<pos->getSize()<<std::endl;
        int nbTets = tets->getSize();
        int nbValidTets = 0;
        for(int i = 0; i < nbTets; i++)
        {
            if(array[i].tets[0] != -1 && array[i].tets[1] != -1 &&
               array[i].tets[2] != -1 && array[i].tets[3] != -1)
            {
                nbValidTets++;
                if(nbValidTets < 20)
                {
                    std::cout<<"Indices : ["<<array[i].verts[0]
                             <<","<<array[i].verts[1]<<","
                             <<array[i].verts[2]<<","
                             <<array[i].verts[3]<<"]"<<std::endl;
                    std::cout<<"Tet valid : "<<std::endl;
                    std::cout<<"Vert 1 ["<<particles[array[i].verts[0] * 3]
                            <<" , "<<particles[array[i].verts[0] * 3 + 1]
                            <<" , "<<particles[array[i].verts[0] * 3 + 2]<<"]"<<std::endl;
                    std::cout<<"Vert 2 ["<<particles[array[i].verts[1] * 3]
                            <<" , "<<particles[array[i].verts[1] * 3 + 1]
                            <<" , "<<particles[array[i].verts[1] * 3 + 2]<<"]"<<std::endl;
                    std::cout<<"Vert 3 ["<<particles[array[i].verts[2] * 3]
                            <<" , "<<particles[array[i].verts[2] * 3 + 1]
                            <<" , "<<particles[array[i].verts[2] * 3 + 2]<<"]"<<std::endl;
                    std::cout<<"Vert 4 ["<<particles[array[i].verts[3] * 3]
                            <<" , "<<particles[array[i].verts[3] * 3 + 1]
                            <<" , "<<particles[array[i].verts[3] * 3 + 2]<<"]"<<std::endl;
                }
            }
        }
        std::cout<<"The container has "<<nbValidTets<<" valid tets."<<std::endl;
        return;
    }
    else
    {
        std::cerr<<"ERROR : tets not found in the map"<<std::endl;
        return;
    }
}

void countNbTetsBlock(dblock_t* block)
{
    tet_t* array = block->tets;
    int nbTets = block->num_tets;
    int nbValidTets = 0;
    for(int i = 0; i < nbTets; i++)
    {
        if(array[i].tets[0] != -1 && array[i].tets[1] != -1 &&
           array[i].tets[2] != -1 && array[i].tets[3] != -1)
        {
            nbValidTets++;
            if(nbValidTets < 20)
            {
                std::cout<<"Tet valid : "<<std::endl;
                std::cout<<"Indices : ["<<array[i].verts[0]
                         <<","<<array[i].verts[1]<<","
                         <<array[i].verts[2]<<","
                         <<array[i].verts[3]<<"]"<<std::endl;
                std::cout<<"Vert 1 ["<<block->particles[array[i].verts[0] * 3]
                        <<" , "<<block->particles[array[i].verts[0] * 3 + 1]
                        <<" , "<<block->particles[array[i].verts[0] * 3 + 2]<<"]"<<std::endl;
                std::cout<<"Vert 2 ["<<block->particles[array[i].verts[1] * 3]
                        <<" , "<<block->particles[array[i].verts[1] * 3 + 1]
                        <<" , "<<block->particles[array[i].verts[1] * 3 + 2]<<"]"<<std::endl;
                std::cout<<"Vert 3 ["<<block->particles[array[i].verts[2] * 3]
                        <<" , "<<block->particles[array[i].verts[2] * 3 + 1]
                        <<" , "<<block->particles[array[i].verts[2] * 3 + 2]<<"]"<<std::endl;
                std::cout<<"Vert 4 ["<<block->particles[array[i].verts[3] * 3]
                        <<" , "<<block->particles[array[i].verts[3] * 3 + 1]
                        <<" , "<<block->particles[array[i].verts[3] * 3 + 2]<<"]"<<std::endl;
            }

        }
    }
    std::cout<<"The block has "<<nbValidTets<<" valid tets."<<std::endl;
    return;
}

void checkTets(std::shared_ptr<ConstructData> container, dblock_t* block)
{
    std::shared_ptr<BaseConstructData> field = container->getData("tets");
    if(field)
    {
        std::shared_ptr<ArrayTetsData > tets = dynamic_pointer_cast<ArrayTetsData>(field);
        tet_t* arrayContainer = tets->getArray();
        int nbTetsContainer = tets->getSize();
        tet_t* arrayBlock = block->tets;
        int nbTetsBlock = block->num_tets;


        int nbValidTets = 0;
        std::cout<<"Tets in container : "<<nbTetsContainer<<", Tets in block : "<<nbTetsBlock<<std::endl;


        return;
    }
    else
    {
        std::cerr<<"ERROR : tets not found in the map"<<std::endl;
        return;
    }
}

void checkPosContainer(std::shared_ptr<ConstructData> container)
{
    std::shared_ptr<BaseConstructData> field = container->getData("pos");
    if(field)
    {
        std::shared_ptr<ArrayConstructData<float> > pos =
                dynamic_pointer_cast<ArrayConstructData<float> >(field);
        int nbParticles = pos->getNbItems();
        float* array = pos->getArray();
        std::cout<<"Invalid particles  [";
        for(int i = 0; i< nbParticles; i++)
        {
            if(array[3*i] < 0.0001f && array[3*i] > -0.0001f)
            {
                std::cout<<i<<",";
            }
        }
        std::cout<<"]"<<std::endl;

    }
    else
    {
        std::cerr<<"ERROR : pos not found in the map."<<std::endl;
    }
}

void checkPosBlock(dblock_t* block)
{
    int nbParticles = block->num_particles;
    float* array = block->particles;
    std::cout<<"Invalid particles block [";
    for(int i = 0; i< nbParticles; i++)
    {
        if(array[3*i] < 0.0001f && array[3*i] > -0.0001f)
        {
            std::cout<<i<<",";
        }
    }
    std::cout<<"]"<<std::endl;
}


// 3d point or vector
struct vec3d {
  float x, y, z;
};

int nblocks;
diy::Master* master;

// global data extents
vec3d data_min, data_max;

int main(int argc,
         char** argv)
{
    //Loading the data
    int tot_blocks; // total number of blocks

    diy::mpi::environment	    env(argc, argv);
    diy::mpi::communicator    world;

    diy::Master               master(world, -1, -1,
                                     &create_block,
                                     &destroy_block);

    diy::ContiguousAssigner   assigner(world.size(), -1);	    // number of blocks will be set by read_blocks()
    diy::io::read_blocks(argv[1], world, assigner, master, &load_block_light);

    tot_blocks = nblocks = master.size();
    ::master = &master;

    std::cout<<"Blocks read: "<<tot_blocks<<std::endl;

    // get overall data extent
    for (int i = 0; i < nblocks; i++) {
      if (i == 0) {
        data_min.x = master.block<dblock_t>(i)->mins[0];
        data_min.y = master.block<dblock_t>(i)->mins[1];
        data_min.z = master.block<dblock_t>(i)->mins[2];
        data_max.x = master.block<dblock_t>(i)->maxs[0];
        data_max.y = master.block<dblock_t>(i)->maxs[1];
        data_max.z = master.block<dblock_t>(i)->maxs[2];
      }
      if (master.block<dblock_t>(i)->mins[0] < data_min.x)
        data_min.x = master.block<dblock_t>(i)->mins[0];
      if (master.block<dblock_t>(i)->mins[1] < data_min.y)
        data_min.y = master.block<dblock_t>(i)->mins[1];
      if (master.block<dblock_t>(i)->mins[2] < data_min.z)
        data_min.z = master.block<dblock_t>(i)->mins[2];
      if (master.block<dblock_t>(i)->maxs[0] > data_max.x)
        data_max.x = master.block<dblock_t>(i)->maxs[0];
      if (master.block<dblock_t>(i)->maxs[1] > data_max.y)
        data_max.y = master.block<dblock_t>(i)->maxs[1];
      if (master.block<dblock_t>(i)->maxs[2] > data_max.z)
        data_max.z = master.block<dblock_t>(i)->maxs[2];
    }

    // debug
    fprintf(stderr, "data sizes mins[%.3f %.3f %.3f] maxs[%.3f %.3f %.3f]\n",
        data_min.x, data_min.y, data_min.z,
        data_max.x, data_max.y, data_max.z);

    //Data are loaded, we can split them

//    /* delaunay tessellation for one DIY block */
//    struct dblock_t {

//      int gid;                   /* global block id */  // global
//      float mins[3], maxs[3];    /* block extents */    // global
//      struct bb_c_t data_bounds; /* global data extents */  //skip
//      struct bb_c_t box;	     /* current box; used in swap-reduce() when distributing particles */ //skip
//      void* Dt;                  /* native delaunay data structure */ //skip

//      /* input particles */
//      int num_orig_particles;    /* number of original particles in this block //global
//                                    before any neighbor exchange */
//      int num_particles;         /* current number of particles in this block after any //global
//                                    neighbor exchange; original particles appear first
//                                    followed by received particles */
//      float* particles;          /* all particles, original plus those received from neighbors */   //local (origin + ghost)

//      /* tets */
//      int num_tets;              /* number of delaunay tetrahedra */
//      struct tet_t* tets;        /* delaunay tets */
//      int* rem_gids;             /* owners of remote particles */       //skip
//      int* rem_lids;	     /* "local ids" of the remote particles */  //skip
//      int* vert_to_tet;          /* a tet that contains the vertex */   // size of num_particules Just keep num_origin

//      /* sent particles and convex hull particles
//         these persist between phases of the algorithm but ar not saved in the final output
//         using void* for each so that C files tess-qhull.c and io.c can see them */
//      void* convex_hull_particles;  //skip
//      void* sent_particles;         //skip

//      /* estimated density field */
//      float* density;            /* density field */                        //skip
//      int num_grid_pts;          /* total number of density grid points */  //skip

//      int complete;                                                         //skip
//    };

    //Global container which stores the aggregation of all the local block
    std::vector<std::shared_ptr<ConstructData> > blocks;
    for(int i = 0; i < nblocks; i++)
    {
        //Local container. Will be merged with the global one
        std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
        dblock_t* block = master.block<dblock_t>(i);

        /*std::cout<<"===================================="<<std::endl;
        std::cout<<"Test of the block "<<i<<std::endl;
        std::cout<<"===================================="<<std::endl;
        countNbTetsBlock(block);*/


        //GID
        std::shared_ptr<SimpleConstructData<int> > gid  = std::make_shared<SimpleConstructData<int> >( block->gid, container->getMap() );
        container->appendData("gid", gid,
                              DECAF_NOFLAG, DECAF_SHARED,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

        //Num Particles
        std::shared_ptr<SimpleConstructData<int> > numParticles  = std::make_shared<SimpleConstructData<int> >( block->num_particles, container->getMap() );
        container->appendData("num_particles", numParticles,
                              DECAF_NOFLAG, DECAF_SHARED,
                              DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);

        //Num Orign Particles
        std::shared_ptr<SimpleConstructData<int> > numOrigParticles  = std::make_shared<SimpleConstructData<int> >( block->num_orig_particles, container->getMap() );
        container->appendData("num_orig_particles", numOrigParticles,
                              DECAF_NOFLAG, DECAF_SHARED,
                              DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);

        //BBox
        std::shared_ptr<BBoxConstructData> box = std::make_shared<BBoxConstructData>(block->mins, block->maxs);
        container->appendData("bbox", box,
                              DECAF_NOFLAG, DECAF_SHARED,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

        //Particle positions
        std::shared_ptr<ArrayConstructData<float> > pos =
                std::make_shared<ArrayConstructData<float> >(
                    block->particles, block->num_particles * 3, 3, false, container->getMap());
        container->appendData("pos", pos,
                              DECAF_POS, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        //Num tets
        std::shared_ptr<TetCounterData> numTets  = std::make_shared<TetCounterData>( block->num_tets, container->getMap() );
        container->appendData("num_tets", numTets,
                              DECAF_NOFLAG, DECAF_SHARED,
                              DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        std::cout<<"Number of tets : "<<block->num_tets<<std::endl;

        //Tets
        std::shared_ptr<ArrayTetsData> tets = std::make_shared<ArrayTetsData>(
                    block->tets, block->num_tets, 1, false, container->getMap());
        container->appendData("tets", tets,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

        //Table Vertices to tet
        std::shared_ptr<VertToTetData> verttotet = std::make_shared<VertToTetData>(
                    block->vert_to_tet, block->num_orig_particles, 1, false, container->getMap());
        container->appendData("verttotet", verttotet,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

        //We have to merge first the tets, and then the particles and then the bbox.
        //No split order specific
        std::vector<std::string> mergeOrder =
        {"gid","num_orig_particles","num_particles","pos","tets","num_tets", "bbox","verttotet"};
        container->setMergeOrder(mergeOrder);
        std::vector<std::string> splitOrder =
        {"gid","num_orig_particles","num_particles","num_tets","pos","tets","bbox","verttotet"};
        container->setSplitOrder(splitOrder);

        std::cout<<"Test d'acces aux donnees : "<<std::endl;
        std::cout<<"Acces au tableau de pos... ";
        container->getData(std::string("pos"));
        std::cout<<"OK"<<std::endl;
        std::cout<<"Acces a la table de tets... ";
        container->getData(std::string("verttotet"));
        std::cout<<"OK"<<std::endl;
        std::cout<<"Number of elements the array of verts : "
                 <<container->getData(std::string("verttotet"))->getNbItems()<<std::endl;
        std::cout<<"Fin des tests d'acces aux donnees."<<std::endl;

        std::cout<<"===================================="<<std::endl;
        std::cout<<"Test of the container "<<i<<std::endl;
        std::cout<<"===================================="<<std::endl;
        //countNbTetsContainer(container);
        checkPosContainer(container);

        //Debug
        //dblock_t* b = static_cast<dblock_t*>(create_block());
        //containerToBlock(container, b);
        //printBlock(b);

        blocks.push_back(container);

    }

    //Merging the blocks
    std::cout<<"All the blocks have been creating. Merging..."<<std::endl;
    for(int i = 1; i < blocks.size(); i++)
    {
        std::cout<<"Merging with "<<i<<"..."<<std::endl;
        std::cout<<"Test d'acces aux donnees pour le block 0 : "<<std::endl;
        std::cout<<"Acces au tableau de pos... ";
        blocks[0]->getData(std::string("pos"));
        std::cout<<"OK"<<std::endl;
        std::cout<<"Acces a la table de tets... ";
        blocks[0]->getData(std::string("verttotet"));
        std::cout<<"OK"<<std::endl;
        std::cout<<"Number of elements the array of verts : ";
        std::cout<<blocks[0]->getData(std::string("verttotet"))->getNbItems()<<std::endl;
        std::cout<<"Fin des tests d'acces aux donnees."<<std::endl;
        std::cout<<"Test d'acces aux donnees pour le block "<<i<<" : "<<std::endl;
        std::cout<<"Acces au tableau de pos... ";
        blocks[i]->getData(std::string("pos"));
        std::cout<<"OK"<<std::endl;
        std::cout<<"Number of particules : ";
        std::cout<<blocks[i]->getData(std::string("pos"))->getNbItems()<<std::endl;
        std::cout<<"Acces a la table de tets... ";
        blocks[i]->getData(std::string("verttotet"));
        std::cout<<"OK"<<std::endl;
        std::cout<<"Number of elements the array of verts : ";
        std::cout<<blocks[i]->getData(std::string("verttotet"))->getNbItems()<<std::endl;
        std::cout<<"Fin des tests d'acces aux donnees."<<std::endl;

        blocks[0]->merge(blocks[i]);
        checkPosContainer(blocks[0]);
        std::cout<<"Merging with "<<i<<" done."<<std::endl;
    }
    std::cout<<"All blocks have been merged."<<std::endl;

    if(blocks.size() > 0)
    {

        //Generate an empty block which will be filled just after
        dblock_t* b = static_cast<dblock_t*>(create_block());
        Bounds domain;  //No need to initialize, the data will be overwritten
        Bounds core;    //No need to initialize, the data will be overwritten
        b->gid = 0;
        b->mins[0] = core.min[0]; b->mins[1] = core.min[1]; b->mins[2] = core.min[2];
        b->maxs[0] = core.max[0]; b->maxs[1] = core.max[1]; b->maxs[2] = core.max[2];
        b->data_bounds = domain;
        b->num_orig_particles = 0;
        b->num_particles = 0;
        b->particles = NULL;
        b->num_tets = 0;
        b->tets = NULL;
        b->rem_gids = NULL;
        b->rem_lids = NULL;
        b->vert_to_tet = NULL;
        b->num_grid_pts = 0;
        b->density = NULL;

        std::cout<<"Global containter setup."<<std::endl;
        countNbTetsContainer(blocks.at(0));

        containerToBlock(blocks.at(0), b);
        countNbTetsBlock(b);
        checkTets(blocks.at(0), b);
        printBlock(b);
        std::cout<<"Checking the invalid particles in the final block : "<<std::endl;
        checkPosBlock(b);

        std::cout<<"Clearing the master..."<<std::endl;
        master.clear();
        std::cout<<"Master cleared. Adding the merged block..."<<std::endl;
        diy::Link l;
        master.add(b->gid, b, &l);
        std::cout<<"Merged block added."<<std::endl;

        std::cout<<"Checking the block in the master : "<<std::endl;
        checkPosBlock(master.block<dblock_t>(0));
        countNbTetsBlock(master.block<dblock_t>(0));


        //Storing the data into
        std::cout<<"Writing the merged block..."<<std::endl;
        diy::io::write_blocks("data.out", master.communicator(), master, &save_block_light);
        std::cout<<"Writing completed."<<std::endl;
    }
    else
        std::cout<<"No block has been read or merged."<<std::endl;
    return 0;
}
