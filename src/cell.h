#ifndef CELL_H
#define CELL_H

struct Cell
{
    int edge_id[4];
    bool edge_exist[4];
    bool exist = false;
};

#endif // CELL_H
