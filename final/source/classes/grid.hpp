#ifndef __GRID_H
#define __GRID_H

#include <vector>
#include <glm.hpp>

#include "triangle.hpp"

class Grid {
public:

    // Underlying vector
    std::vector<Triangle *> tris;
    // Special triangles - large
    std::vector<Triangle *> statics;
    // Max width of a triangle
    float triSize;

    //Highest and lowest positions of triangles
    float high;
    float low;

    Grid(std::vector<Triangle *> trs, std::vector<Triangle *> stats, float trSize);

    //quickSort over all triangles
    void initialSort(void);

    bool compare(Triangle *t1, Triangle *t2);
    int partition(unsigned int left, unsigned int right, unsigned int pivot);
    void quicksort(unsigned int left, unsigned int right);

    //Timestep each triangle and insertSort into order
    void rebalance(float stepTime);

    void fixCollisions(void);

    void print(void);

    //Cleans up
	~Grid(void);
};

#endif
