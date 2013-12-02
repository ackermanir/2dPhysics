#ifndef __GRID_H
#define __GRID_H

#include <vector>

#ifdef _WIN32
#include <glm.hpp>
#endif

#include "triangle.hpp"

class Grid {
public:

    // Underlying vector of triangles that move
    std::vector<Triangle *> tris;
    // Special triangles - large
    std::vector<Triangle *> statics;
    // Max width of a triangle
    float triSize;

    std::vector<unsigned int> indices;

    //Highest and lowest positions of triangles
    float high;
    float low;

    Grid(std::vector<Triangle> &trs, std::vector<Triangle> &stats, float trSize);

    //quickSort over all triangles
    void initialSort(void);

    bool compare(Triangle *t1, Triangle *t2);
    int partition(unsigned int left, unsigned int right, unsigned int pivot);
    void quicksort(unsigned int left, unsigned int right);

    //Timestep over all triangles and simulate forces
    void stepAll(float stepTime);
    //Collide all triangles together
    void rebalance(void);

    static void *pThreadStrip(void * input);
    //Iterate over strips and do collisions
    void *itterateOnStrip(int start, int size, int stride);

    void print(void);

    //Cleans up
	~Grid(void);
};

struct PThreadInfo {
    int size;
    int start;
    int stride;
    Grid* grid;
} ;

#endif
