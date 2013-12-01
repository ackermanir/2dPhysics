#ifndef __CLINTER_H
#define __CLINTER_H

#include <string>
#include <algorithm>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <cmath>
#include <time.h>
#include <math.h>
#include <vector>
//OpenCL
#include <CL/cl.h>
#include "clhelp.h"

#include "triangle.hpp"

class ClInter {
public:

    cl_int err;

    std::string sort_name_str;
    std::string setup_name_str;
    std::string max_name_str;
    std::string max2_name_str;
    std::string time_step_name_str;

    std::map<std::string, cl_kernel>
        kernel_map;

    cl_vars_t cv;

    cl_mem g_tri;
    cl_mem g_tri_stat;
    cl_mem g_sort_even;
    cl_mem g_sort_odd;
    cl_mem g_was_swap;
    cl_mem *g_maxx;
    cl_mem g_max;

    unsigned int n;
    unsigned int nStat;

    size_t global_work_size[1];
    size_t local_work_size[1];

    //Create interface, initialize and compile program
    ClInter(std::vector<Tri_g> &trisG, std::vector<Tri_g> &staticsG);

    void setup(void);

    void timeStep(float delta);
    void findMax(void);
    void simulate(float delta);
};


#endif
