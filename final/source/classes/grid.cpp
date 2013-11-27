#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
#include <unordered_map>
#include <iterator>

#include "grid.hpp"
#include <omp.h>

#define LINEAR 0


/* Copy over inputs. Call initialSort */
Grid::Grid(std::vector<Triangle *> trs, std::vector<Triangle *> stats, float trSize)
    :tris(trs), statics(stats) {
    indices = std::vector<unsigned int>();
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

    //discover indices
    indices.clear();
    indices.push_back(0);
    int prevDiv;
    prevDiv = (int)((high - tris[0]->midPt()[1]) / triSize);
    for (unsigned int i = 1; i < tris.size(); i++) {
        int div = (int)((high - tris[i]->midPt()[1]) / triSize);
        if (div != prevDiv) {
            indices.push_back(i);
            prevDiv = div;
        }
    }
}

void Grid::itterateOnStrip(int i) {
    //Iterate over each row strip
    unsigned int off = indices[i];
    unsigned int next = tris.size();
    if (i < indices.size() - 1) {
        //This handles case of there is no next, so go to tris.size()
        next = indices[i+1];
    }
    unsigned int nextNext = tris.size();
    if (i < indices.size() - 2) {
        //again handle case that next row is last row
        nextNext = indices[i+2];
    }

    int ownRowLow = off;
    float orlMid;
    if (off != tris.size()) {
        orlMid = tris.at(ownRowLow)->midPt()[0];
    }
    int nextRowLow;
    float nrlMid;
    if (next != tris.size()) {
        nextRowLow = next;
        nrlMid = tris.at(nextRowLow)->midPt()[0];
    }

    //Iterate over each triangle
    for (unsigned int j = off; j < next; j++) {
        Triangle * tr1 = tris.at(j);

        while (orlMid < (tr1->midPt()[0] - (triSize * 2.0f))) {
            ownRowLow++;
            orlMid = tris.at(ownRowLow)->midPt()[0];
        }
        //collide within own row
        for (unsigned int k = ownRowLow; k < j; k++) {
            Triangle * tr2 = tris.at(k);
            Collision * col = tr1->testColliding(tr2);
            Triangle::handleCollisions(col);
            delete col;
        }

        //Collide with statics
        for (unsigned int k = 0; k < statics.size(); k++) {
            Triangle * tr2 = statics.at(k);
            Collision * col = tr1->testColliding(tr2);
            Triangle::handleCollisions(col);
            delete col;
        }

        //collide with row below
        if (i != indices.size() - 1) {
            while (nrlMid < (tr1->midPt()[0] - (triSize * 2.0f))
                   && nextRowLow < (tris.size() - 1)) {
                nextRowLow++;
                nrlMid = tris.at(nextRowLow)->midPt()[0];
            }

            for (unsigned int k = nextRowLow; k < nextNext; k++) {
                Triangle * tr2 = tris.at(k);
                Collision * col = tr1->testColliding(tr2);
                Triangle::handleCollisions(col);
                delete col;
            }
        }
    }
}

// Simulate all triangles falling
void Grid::stepAll(float stepTime) {
    for (unsigned int i = 0; i < tris.size(); i++) {
        tris[i]->timeStep(stepTime);
    }
}

//Timestep each triangle and insertSort into order
void Grid::rebalance() {
    if (LINEAR) {
        for (unsigned int j = 0; j < tris.size(); j++) {
            Triangle * tr1 = tris.at(j);
            //collide with everyone less than you
            for (unsigned int k = 0; k < j; k++) {
                Triangle * tr2 = tris.at(k);
                Collision * col = tr1->testColliding(tr2);
                Triangle::handleCollisions(col);
                // Here we cleanup collision as it was generated on heap
                delete col;
            }
            //collide with statics
            for (unsigned int k = 0; k < statics.size(); k++) {
                Triangle * tr2 = statics.at(k);
                Collision * col = tr1->testColliding(tr2);
                Triangle::handleCollisions(col);
                delete col;
            }
        }
        return;
    }

    for (int i = 0; i < indices.size(); i++) {
        itterateOnStrip(i);
    }
// #pragma omp parallel for ordered schedule(dynamic) num_threads(2)
//     for (int i = 0; i < indices.size(); i += 2) {
//         itterateOnStrip(i);
//     }

// #pragma omp parallel for ordered schedule(dynamic) num_threads(2)
//     for (int i = 1; i < indices.size(); i += 2) {
//         itterateOnStrip(i);
//     }
}

void Grid::print(void) {
    std::cout << "Grid's triangle midpoints:\n";
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int off = indices[i];
        unsigned int next;
        if (i != indices.size() - 1) {
            next = indices[i+1];
        } else {
            next = tris.size();
        }
        for (unsigned int j = off; j < next; j++) {
            glm::vec2 mid = tris[j]->midPt();
            std::cout << "(" << mid[0] << " " << mid[1] << ") ";
        }
        std::cout << "\n";
    }
}

//Cleans up
Grid::~Grid(void) {
}
