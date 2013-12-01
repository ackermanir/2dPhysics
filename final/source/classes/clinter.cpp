#include "clinter.hpp"

/* Triangle definitions. */
ClInter::ClInter(std::vector<Tri_g> &trisG, std::vector<Tri_g> &staticsG) {
    n = trisG.size();
    nStat = staticsG.size();
    global_work_size[0] = n;
    local_work_size[0] = 128;

    std::string kernel_source_str;

    std::string arraycompact_kernel_file =
        std::string("source/collide.cl");

    std::list<std::string> kernel_names;
    sort_name_str = std::string("sort");
    setup_name_str = std::string("setup");
    max_name_str = std::string("max_height");
    max2_name_str = std::string("max_height2");
    time_step_name_str = std::string("time_step");

    kernel_names.push_back(sort_name_str);
    kernel_names.push_back(setup_name_str);
    kernel_names.push_back(max_name_str);
    kernel_names.push_back(max2_name_str);
    kernel_names.push_back(time_step_name_str);

    readFile(arraycompact_kernel_file,
             kernel_source_str);

    initialize_ocl(cv);

    compile_ocl_program(kernel_map, cv,
                        kernel_source_str.c_str(),
                        kernel_names);

    unsigned int first_swap = 0;

    err = CL_SUCCESS;
    g_tri = clCreateBuffer(cv.context,CL_MEM_READ_WRITE,
                           sizeof(Tri_g)*n,NULL,&err);
    CHK_ERR(err);

    g_tri_stat = clCreateBuffer(cv.context,CL_MEM_READ_WRITE,
                                sizeof(Tri_g)*nStat,NULL,&err);
    CHK_ERR(err);

    g_sort_even = clCreateBuffer(cv.context,CL_MEM_READ_WRITE,
                                 sizeof(cl_uint)*n,NULL,&err);
    CHK_ERR(err);

    g_sort_odd = clCreateBuffer(cv.context,CL_MEM_READ_WRITE,
                                sizeof(cl_uint)*n,NULL,&err);
    CHK_ERR(err);

    g_was_swap = clCreateBuffer(cv.context, CL_MEM_READ_WRITE,
                                sizeof(cl_uint), NULL, &err);
    CHK_ERR(err);

    //setup clmems used by max
    unsigned int start_size = global_work_size[0];
    unsigned int left_over = (start_size + local_work_size[0] - 1)
        / local_work_size[0];
    unsigned int max_length = 1;
    while (left_over > 1) {
        max_length++;
        start_size = left_over;
        left_over = (start_size + local_work_size[0] - 1)
            / local_work_size[0];
    }
    std::cout << global_work_size[0] << "\n";
    std::cout << local_work_size[0] << "\n";
    std::cout << max_length << "\n";
    g_maxx = new cl_mem[max_length];
    left_over = (global_work_size[0] + local_work_size[0] - 1)
        / local_work_size[0];
    for (unsigned int i = 0; i < max_length; i++) {
        g_maxx[i] = clCreateBuffer(cv.context, CL_MEM_READ_WRITE,
                                   sizeof(cl_float)*left_over, NULL, &err);
        CHK_ERR(err);
        left_over = (left_over + local_work_size[0] - 1)
            / local_work_size[0];
    }

    //copy data from host CPU to GPU
    err = clEnqueueWriteBuffer(cv.commands, g_tri, true, 0,
                               sizeof(Tri_g)*n,
                               &trisG[0], 0, NULL, NULL);
    CHK_ERR(err);

    err = clEnqueueWriteBuffer(cv.commands, g_tri_stat, true, 0,
                               sizeof(Triangle)*nStat,
                               &staticsG[0], 0, NULL, NULL);
    CHK_ERR(err);

    err = clEnqueueWriteBuffer(cv.commands, g_was_swap, true, 0,
                               sizeof(cl_uint),
                               &first_swap, 0, NULL, NULL);
    CHK_ERR(err);
}

//Initially setup pointer arrays
void ClInter::setup(void) {
    cl_kernel setup_kern = kernel_map[setup_name_str];

    err = clSetKernelArg(setup_kern, 0, sizeof(cl_mem), &g_tri);
    CHK_ERR(err);

    err = clSetKernelArg(setup_kern, 1, sizeof(cl_mem), &g_sort_even);
    CHK_ERR(err);

    err = clSetKernelArg(setup_kern, 2, sizeof(cl_mem), &g_sort_odd);
    CHK_ERR(err);

    err = clSetKernelArg(setup_kern, 3, sizeof(cl_uint), &n);
    CHK_ERR(err);

    // call setup kernel, sets up sort_even and sort_odd
    err = clEnqueueNDRangeKernel(cv.commands,
                                 setup_kern,
                                 1, //work_dim,
                                 NULL, //global_work_offset
                                 global_work_size,
                                 local_work_size,
                                 0, //num_events_in_wait_list
                                 NULL, //event_wait_list
                                 NULL //
                                 );

    CHK_ERR(err);

    err = clFinish(cv.commands);
    CHK_ERR(err);

    //double check outputs
    // unsigned int* dub = new unsigned int[n];
    // err = clEnqueueReadBuffer(cv.commands, g_sort_even, true,
    //                           0, sizeof(cl_uint)*n,
    //                           dub, 0, NULL, NULL);
    // CHK_ERR(err);
    // for (unsigned int i = 0; i < n; i++) {
    //     std::cout << dub[i] << " ";
    // }
    // std::cout << "\n";
    // delete dub;
}

void ClInter::timeStep(float delta) {
    cl_kernel time_kern = kernel_map[time_step_name_str];

    err = clSetKernelArg(time_kern, 0, sizeof(cl_mem), &g_tri);
    CHK_ERR(err);
    err = clSetKernelArg(time_kern, 1, local_work_size[0]*sizeof(Tri_g), NULL);
    CHK_ERR(err);
    err = clSetKernelArg(time_kern, 2, sizeof(cl_float), &delta);
    CHK_ERR(err);
    err = clSetKernelArg(time_kern, 3, sizeof(cl_uint), &global_work_size[0]);
    CHK_ERR(err);

    err = clEnqueueNDRangeKernel(cv.commands,
                                 time_kern,
                                 1, //work_dim,
                                 NULL, //global_work_offset
                                 global_work_size,
                                 local_work_size,
                                 0, //num_events_in_wait_list
                                 NULL, //event_wait_list
                                 NULL //
                                 );
    CHK_ERR(err);
    err = clFinish(cv.commands);
    CHK_ERR(err);
}

void ClInter::findMax(void) {
    cl_kernel max_kern = kernel_map[max_name_str];

    unsigned int start_size = global_work_size[0];
    unsigned int left_over = (start_size + local_work_size[0] - 1)
        / local_work_size[0];
    unsigned int max_num = 0;

    err = clSetKernelArg(max_kern, 0, sizeof(cl_mem), &g_tri);
    CHK_ERR(err);
    err = clSetKernelArg(max_kern, 1, sizeof(cl_mem), &g_maxx[0]);
    CHK_ERR(err);
    err = clSetKernelArg(max_kern, 2, local_work_size[0]*sizeof(cl_float), NULL);
    CHK_ERR(err);
    err = clSetKernelArg(max_kern, 3, sizeof(cl_uint), &start_size);
    CHK_ERR(err);

    err = clEnqueueNDRangeKernel(cv.commands,
                                 max_kern,
                                 1, //work_dim,
                                 NULL, //global_work_offset
                                 global_work_size,
                                 local_work_size,
                                 0, //num_events_in_wait_list
                                 NULL, //event_wait_list
                                 NULL //
                                 );
    CHK_ERR(err);

    while (left_over > 1) {
        cl_kernel max2_kern = kernel_map[max2_name_str];

        start_size = left_over;
        left_over = (start_size + local_work_size[0] - 1)
            / local_work_size[0];
        max_num++;

        err = clSetKernelArg(max2_kern, 0, sizeof(cl_mem), &g_maxx[max_num - 1]);
        CHK_ERR(err);
        err = clSetKernelArg(max2_kern, 1, sizeof(cl_mem), &g_maxx[max_num]);
        CHK_ERR(err);
        err = clSetKernelArg(max2_kern, 2, local_work_size[0]*sizeof(cl_float), NULL);
        CHK_ERR(err);
        err = clSetKernelArg(max2_kern, 3, sizeof(cl_uint), &start_size);
        CHK_ERR(err);

        err = clEnqueueNDRangeKernel(cv.commands,
                                     max_kern,
                                     1, //work_dim,
                                     NULL, //global_work_offset
                                     global_work_size,
                                     local_work_size,
                                     0, //num_events_in_wait_list
                                     NULL, //event_wait_list
                                     NULL //
                                     );
        CHK_ERR(err);
    }
    g_max = g_maxx[max_num];

    //double check output
    float answer;
    err = clEnqueueReadBuffer(cv.commands, g_maxx[0], true,
                              0, sizeof(cl_float),
                              &answer, 0, NULL, NULL);
    CHK_ERR(err);
    std::cout << "Max of all is " << answer << "\n";
    std::cout << "Max index is " << max_num << "\n";
}

void ClInter::simulate(float delta) {
    // timeStep(delta);

    // findMax();

    //////sort triangles

    // cl_kernel sort_kern = kernel_map[sort_name_str];

    // err = clSetKernelArg(sort_kern, 0, sizeof(cl_mem), &g_tri);
    // CHK_ERR(err);
    // err = clSetKernelArg(sort_kern, 1, sizeof(cl_mem), &g_sort_even);
    // CHK_ERR(err);
    // err = clSetKernelArg(sort_kern, 2, sizeof(cl_mem), &g_bmax);
    // CHK_ERR(err);
    // err = clSetKernelArg(sort_kern, 3, sizeof(cl_mem), &g_was_swap);
    // CHK_ERR(err);
    // err = clSetKernelArg(sort_kern, 4, local_work_size[0]*sizeof(Tri_g), NULL);
    // CHK_ERR(err);
    // err = clSetKernelArg(sort_kern, 5, local_work_size[0]*sizeof(cl_uint), NULL);
    // CHK_ERR(err);
    // float dub_tri_size = width * 4.0f;
    // err = clSetKernelArg(sort_kern, 6, sizeof(cl_float), &dub_tri_size);
    // CHK_ERR(err);
    // err = clSetKernelArg(sort_kern, 7, sizeof(cl_uint), &global_work_size[0]);
    // CHK_ERR(err);

    // err = clEnqueueNDRangeKernel(cv.commands,
    //                              sort_kern,
    //                              1,//work_dim,
    //                              NULL, //global_work_offset
    //                              global_work_size, //global_work_size
    //                              local_work_size, //local_work_size
    //                              0, //num_events_in_wait_list
    //                              NULL, //event_wait_list
    //                              NULL //
    //                              );

    // CHK_ERR(err);

    // err = clFinish(cv.commands);
    // CHK_ERR(err);

    //////collide
}
