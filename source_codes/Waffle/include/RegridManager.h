#ifndef REGRIDMANAGER_H
#define REGRIDMANAGER_H

#include "Chunk.h"
#include "WaffleIndexManager.h"
#include "Waffle.h"
#include <iostream>

class RegridManager
{
  public:
    RegridManager(WaffleIndexManager *&original_index, WaffleIndexManager *&new_index);
    void operator()();

  private:
    WaffleIndexManager *&original_index;
    WaffleIndexManager *&new_index;
    Object new_object;
};

#endif