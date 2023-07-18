#ifndef REGRIDMANAGER_H
#define REGRIDMANAGER_H

#include "Chunk.h"
#include "GridIndexManager.h"
#include "Waffle.h"
#include <iostream>

class RegridManager
{
  public:
    RegridManager(GridIndexManager *&original_index, GridIndexManager *&new_index);
    void operator()();

  private:
    GridIndexManager *&original_index;
    GridIndexManager *&new_index;
    Object new_object;
};

#endif