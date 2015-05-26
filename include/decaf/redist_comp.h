//---------------------------------------------------------------------------
//
// data interface
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_REDIST_COMP_H
#define DECAF_REDIST_COMP_H

#include "transport/mpi/newData.hpp"
#include <vector>
#include <memory>
#include <iostream>

namespace decaf
{

  // This class defines the common interface for the redistribution component (MxN)
  // This interface is independant from the datatype or the transport
  // implementation. Specialized components will implement the redistribution
  // in fonction of transport layer
  class RedistComp
  {
  public:
      RedistComp(){}
      RedistComp(int rankSource, int nbSources,
                 int rankDest, int nbDests) :
          rankSource_(rankSource), nbSources_(nbSources),
          rankDest_(rankDest), nbDests_(nbDests), summerizeDest_(NULL){}

      virtual ~RedistComp(){}

      // Run the pipeline of operations to redistribute the data.
      // This fonction is the only one zhich should be called from
      // the main programm
      virtual void process(shared_ptr<BaseData> data);

      int getRankSource() { return rankSource_; }
      int getNbSources() { return nbSources_; }
      int getRankDest() { return rankDest_; }
      int getNbDest() { return nbDests_; }


  protected:
      // Compute the values necessary to determine how the data should be
      // splitted and redistributed.
      virtual void computeGlobal(shared_ptr<BaseData> data)=0;

      // Seperate the Data into chunks for each destination involve in the
      // component and fill the splitChunks vector
      virtual void splitData(shared_ptr<BaseData> data)=0;

      // Transfert the chunks from the sources to the destination. The data
      // should be stored in the vector receivedChunks
      virtual void redistribute(shared_ptr<BaseData> data)=0;

      // Merge the chunks from the vector receivedChunks into one single Data.
      virtual shared_ptr<BaseData> merge()=0;

      int rankSource_; // Rank of the first source (=sender)
      int nbSources_;  // Number of sources, supposed to be consecutives
      int rankDest_;   // Rank of the first destination (=receiver)
      int nbDests_;     // Number of destinations

      std::vector<std::shared_ptr<BaseData> > splitChunks_;
      std::vector<std::shared_ptr<char> > receivedChunks_;
      int* summerizeDest_;
      std::vector<int> destList_;
  };

} //namespace decaf

void decaf::RedistComp::process(shared_ptr<BaseData> data)
{
    computeGlobal(data);
    std::cout<<"Global values are computed"<<std::endl;

    splitData(data);
    std::cout<<"Data are splited"<<std::endl;

    redistribute(data);

    //merge();
}

#endif
