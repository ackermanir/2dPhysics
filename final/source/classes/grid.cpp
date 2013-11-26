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

//Timestep each triangle and insertSort into order
void Grid::rebalance(float stepTime) {
    initialSort();

    // Simulate all triangles falling
    for (unsigned int i = 0; i < tris.size(); i++) {
        tris[i]->timeStep(stepTime);
    }

    // Linear code
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
    } else {

        //Iterate over each row strip
// #pragma omp parallel for ordered schedule(dynamic) num_threads(8)
        for (int i = 0; i < indices.size(); i++) {
            unsigned int off = indices[i];
            unsigned int next = tris.size();
            if (i != indices.size() - 1) {
                //This handles case of there is no next, so go to tris.size()
                next = indices[i+1];
            }

            unsigned int ownRowOffset = off;

            unsigned int previousRowStartingOffset = 0;
            if (i != 0) {
                previousRowStartingOffset = indices[i - 1];
            }
            unsigned int nextRowStartingOffset = 0;
            if (i != indices.size() - 1) {
                nextRowStartingOffset = indices[i + 1];
            }

            //Iterate over each triangle
            for (unsigned int j = off; j < next; j++) {
                Triangle * tr1 = tris.at(j);

                while (tris.at(ownRowOffset)->midPt()[0] < tr1->midPt()[0] - (2.887 * triSize)
                       && ownRowOffset < j) {
                    ownRowOffset++;
                }

                //collide within own row
                for (unsigned int k = ownRowOffset; k < j; k++) {
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

                //collide with row above
                if (i != 0) {
                    while (tris.at(previousRowStartingOffset)->midPt()[0] < tr1->midPt()[0] - (2.5 * triSize)
                           && previousRowStartingOffset < off) {
                        previousRowStartingOffset++;
                    }

                    for (unsigned int k = previousRowStartingOffset; k < off; k++) {
                        Triangle * tr2 = tris.at(k);
                        if (tr2->midPt()[0] > tr1->midPt()[0] + (2.5 * triSize)) {
                            break;
                        }

                        Collision * col = tr1->testColliding(tr2);
                        Triangle::handleCollisions(col);

                        delete col;
                    }
                }

                //collide with row below
                if (i != indices.size() - 1) {
                    unsigned int nextNext = tris.size();
                    if (i != indices.size() - 2) {
                        nextNext = indices[i+2];
                    }
                    while (tris.at(nextRowStartingOffset)->midPt()[0] < tr1->midPt()[0] - (2.5 * triSize)
                           && nextRowStartingOffset < nextNext - 1) {
                        nextRowStartingOffset++;
                    }

                    for (unsigned int k = next; k < nextNext; k++) {
                        Triangle * tr2 = tris.at(k);
                        if (tr2->midPt()[0] > tr1->midPt()[0] + (2.5 * triSize)) {
                            break;
                        }
                        Collision * col = tr1->testColliding(tr2);
                        Triangle::handleCollisions(col);
                        delete col;
                    }
                }
            }
        }
    }
}

//
void Grid::fixCollisions(void) {
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
