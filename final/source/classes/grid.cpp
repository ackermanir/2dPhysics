#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
#include <unordered_map>
#include <iterator>

#include "grid.hpp"
#include <omp.h>
#include "pthread.h"

#define LINEAR 0
#define NUMTHREADS 1

/* Copy over inputs. Call initialSort */
Grid::Grid(std::vector<Triangle> &trs, std::vector<Triangle> &stats, float trSize) {
    tris = std::vector<Triangle *>();
    for (unsigned int i = 0; i < trs.size(); i++) {
        tris.push_back(&trs[i]);
    }
    statics = std::vector<Triangle *>();
    for (unsigned int i = 0; i < stats.size(); i++) {
        statics.push_back(&stats[i]);
    }
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

void *Grid::pThreadStrip(void * input) {
    PThreadInfo* pti = (PThreadInfo *)input;
    return (pti->grid)->itterateOnStrip(pti->start, pti->size, pti->stride);
}

void *Grid::itterateOnStrip(int start, int size, int stride) {
#pragma omp parallel for ordered schedule(dynamic) num_threads(16)
    for (int i = start; i < start + size; i += stride) {
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
            Triangle tr1 = *tris.at(j);

            while (orlMid < (tr1.midPt()[0] - triSize)) {
                ownRowLow++;
                orlMid = tris.at(ownRowLow)->midPt()[0];
            }
            //collide within own row
            for (unsigned int k = ownRowLow; k < j; k++) {
                Triangle tr2 = *tris.at(k);
                if (tr1.collide(tr2)) {
                    *tris[k] = tr2;
                }
            }

            //Collide with statics
            for (unsigned int k = 0; k < statics.size(); k++) {
                Triangle tr2 = *statics.at(k);
                if (tr1.collide(tr2)) {
                    *statics[k] = tr2;
                }
            }

            //collide with row below
            if (i != indices.size() - 1) {
                while (nrlMid < (tr1.midPt()[0] - triSize)
                       && nextRowLow < (tris.size() - 1)) {
                    nextRowLow++;
                    nrlMid = tris.at(nextRowLow)->midPt()[0];
                }

                for (unsigned int k = nextRowLow; k < nextNext; k++) {
                    Triangle tr2 = *tris.at(k);
                    if (tr1.collide(tr2)) {
                        *tris[k] = tr2;
                    }
                }
            }
            *tris[j] = tr1;
        }
    }
    return NULL;
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
            Triangle tr1 = *tris.at(j);
            //collide with everyone less than you
            for (unsigned int k = 0; k < j; k++) {
                Triangle tr2 = *tris.at(k);
                if (tr1.collide(tr2)) {
                    *tris[k] = tr2;
                }
            }
            //collide with statics
            for (unsigned int k = 0; k < statics.size(); k++) {
                Triangle tr2 = *statics.at(k);
                if (tr1.collide(tr2)) {
                    *statics[k] = tr2;
                }
            }
            *tris[j] = tr1;
        }
        return;
    }

    // single thread version
    itterateOnStrip(0, indices.size(), 2);
    itterateOnStrip(1, indices.size() - 1, 2);

    // int size = indices.size() / NUMTHREADS;
    // pthread_t tid[NUMTHREADS];
    // //contains int of start, then int of length
    // PThreadInfo thrd_info[NUMTHREADS];
    // for (int i = 0; i < NUMTHREADS; i++) {
    //     thrd_info[i].start = i * size;
    //     if (i == 7) {
    //         size = indices.size() - (size * 7);
    //     }
    //     thrd_info[i].size = size;
    //     thrd_info[i].grid = this;
    //     thrd_info[i].stride = 2;
    //     pthread_create(&tid[i], 0, &Grid::pThreadStrip, (void*)&thrd_info[i]);
    // }

    // for (int i = 0; i < NUMTHREADS; i++) {
    //     pthread_join(tid[i], NULL);
    // }

    // size = (indices.size() - 1) / NUMTHREADS;

    // for (int i = 1; i < NUMTHREADS; i++) {
    //     thrd_info[i].start = i * size;
    //     if (i == 7) {
    //         size = indices.size() - (size * 7) - 1;
    //     }
    //     thrd_info[i].size = size;
    //     thrd_info[i].grid = this;
    //     thrd_info[i].stride = 2;
    //     pthread_create(&tid[i], 0, &Grid::pThreadStrip, (void*)&thrd_info[i]);
    // }

    // for (int i = 0; i < NUMTHREADS; i++) {
    //     pthread_join(tid[i], NULL);
    // }

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


// To Try: midpiont being part of triangle so we don't
// have to continually calculate


/// Let's plan for the GPU

// Timestepping:
//     Trivial, simply do it to each of the triangles, 1 per work unit

// Sorting:
//     have 2x (or 3x) size to write to.
//     2x: sort self, write back to self. Write extra to extra slots
//
//     Next times: take in extra slots
//     write out to bool true if 

// Collisions:
//     Group for collisions in 2 wide strip. Do collisions in group of size n, grab additional
//     16 (could probably be lower) per work group.
//     Find pairs of AABB collisions and write out for next phase, or just write to some local cache
