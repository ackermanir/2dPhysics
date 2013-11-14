#include <fstream>
#include <iostream>
#include <math.h>
#include <string>

#include "grid.hpp"

/* Copy over inputs. Call initialSort */
Grid::Grid(std::vector<Triangle *> trs, std::vector<Triangle *> stats, float trSize)
    :tris(trs), statics(stats) {
    triSize = trSize;
    initialSort();
}

// True if t1 is less than or equal to t2
bool Grid::compare(Triangle *t1, Triangle *t2) {
    glm::vec2 mid1 = t1->midPt();
    glm::vec2 mid2 = t2->midPt();
    int index1 = (int)((high - mid1[1]) / triSize);
    int index2 = (int)((high - mid2[1]) / triSize);
    if (index1 < index2) {
        return true;
    } else if (index1 > index2) {
        return false;
    }
    return mid1[0] <= mid2[0];
}

int Grid::partition(unsigned int left, unsigned int right, unsigned int pivot) {
    Triangle *pivotVal = tris[pivot];
    Triangle *temp = tris[right];
    tris[right] = pivotVal;
    tris[pivot] = temp;
    unsigned int mid = left;
    for (unsigned int i = left; i < right; i++) {
        Triangle * ll = tris[i];
        bool cmp = compare(tris[i], pivotVal);
        if (cmp) {
            temp = tris[mid];
            tris[mid] = tris[i];
            tris[i] = temp;
            ++mid;
        }
    }
    temp = tris[right];
    tris[right] = tris[mid];
    tris[mid] = temp;
    return mid;
}

void Grid::quicksort(unsigned int left, unsigned int right) {
    if (left < right) {
        unsigned int pivot = (left + right) / 2;
        pivot = partition(left, right, pivot);
        if (pivot == left) {
            quicksort(left, pivot);
        } else {
            quicksort(left, pivot - 1);
        }
        quicksort(pivot + 1, right);
    }
}


//quickSort over all triangles
void Grid::initialSort(void) {
    if (tris.size() == 0) return;

    //find min and max
    float triY = tris[0]->midPt()[1];
    high = triY;
    low = triY;
    for (unsigned int i = 1; i < tris.size(); i++) {
        triY = tris[i]->midPt()[1];
        if (triY > high) {
            high = triY;
        } else if (triY < low) {
            low = triY;
        }
    }

    //QuickSort in place
    quicksort(0, tris.size() - 1);
}

//Timestep each triangle and insertSort into order
void Grid::rebalance(float stepTime) {
    initialSort();

    //Simulate all triangles falling
    for (unsigned int i = 0; i < tris.size(); i++) {
        tris[i]->timeStep(stepTime);
    }

    //Resolve collisions
    for (unsigned int i = 0; i < tris.size(); i++) {
        Triangle * tr1 = tris.at(i);
        for (unsigned int j = 0; j < i; j++) {
            Triangle * tr2 = tris.at(j);
            Collision * col = tr1->testColliding(tr2);
            Triangle::handleCollisions(col);
            // Here we cleanup collision as it was generated on heap
            delete col;
        }
        //Collide with statics
        for (unsigned int j = 0; j < statics.size(); j++) {
            Triangle * tr2 = statics.at(j);
            Collision * col = tr1->testColliding(tr2);
            Triangle::handleCollisions(col);
            // Here we cleanup collision as it was generated on heap
            delete col;
        }
    }
}

//
void Grid::fixCollisions(void) {
}

void Grid::print(void) {
    std::cout << "Grid's triangle midpoints:\n";
    int prevDiv;
    prevDiv = (int)((high - tris[0]->midPt()[1]) / triSize);
    for (unsigned int i = 0; i < tris.size(); i++) {
        glm::vec2 mid = tris[i]->midPt();
        int div = (int)((high - tris[i]->midPt()[1]) / triSize);
        if (div != prevDiv) {
            std::cout << "\n";
            prevDiv = div;
        }
        std::cout << "(" << mid[0] << " " << mid[1] << ") ";
    }
    std::cout << "\n";
}

//Cleans up
Grid::~Grid(void) {
}
