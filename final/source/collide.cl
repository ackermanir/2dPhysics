typedef struct Tri_g
{
    float2 verts[3];
    float2 velocity;
    float rot_vel;
    float inv_mass;
    uint vert_buf;

} Tri_g;

float2 tri_mid_pt(__local Tri_g* t) {
    float2 mid;
    mid.x = t->verts[0].x + t->verts[1].x + t->verts[2].x;
    mid.y = t->verts[0].y + t->verts[1].y + t->verts[2].y;
    mid.x /= 3.0f;
    mid.y /= 3.0f;
    return mid;
};

bool tri_compare(__local Tri_g* t1, __local Tri_g* t2,
                 float high, float tri_size) {
    float2 mid1 = tri_mid_pt(t1);
    float2 mid2 = tri_mid_pt(t2);
    uint index1 = (uint)((high - mid1.y) / tri_size);
    uint index2 = (uint)((high - mid2.y) / tri_size);
    if (index1 < index2) {
        return true;
    } else if (index1 > index2) {
        return false;
    }
    return mid1.x <= mid2.x;
};

__kernel void setup(__global Tri_g * g_tri,
                    __global uint * g_sort_even,
                    __global uint * g_sort_odd,
                    int n)
{
    size_t idx = get_global_id(0);
    size_t tid = get_local_id(0);
    size_t dim = get_local_size(0);
    size_t gid = get_group_id(0);

    if (idx < n) {
        g_sort_even[idx] = idx;
        g_sort_odd[idx] = idx;
    }
}

//Store max in out from this group
__kernel void max_height(__global Tri_g * g_tri,
                         __global float * g_out,
                         __local float * buf,
                         uint n)
{
    size_t idx = get_global_id(0);
    size_t tid = get_local_id(0);
    size_t dim = get_local_size(0);
    size_t gid = get_group_id(0);

    if (idx < n) {
        Tri_g tr = g_tri[idx];

        //maximize of 3 verts
        buf[tid] = tr.verts[0].y;
        if (tr.verts[1].y > buf[tid]) {
            buf[tid] = tr.verts[1].y;;
        }
        if (tr.verts[2].y > buf[tid]) {
            buf[tid] = tr.verts[2].y;;
        }

        int stride = 1;
        while (stride < dim) {
            barrier(CLK_LOCAL_MEM_FENCE);
            int next = tid + stride;
            if (next < dim && gid + stride < n && buf[next] > buf[tid]) {
                float t = buf[next];
                barrier(CLK_LOCAL_MEM_FENCE);
                buf[tid] = t;
            }
            stride = stride << 2;
        }
        if (tid == 0) {
            g_out[gid] = buf[0];
        }
    }
}

//Store max in out from this group
// takes in a float array as input
__kernel void max_height2(__global float * g_in,
                          __global float * g_out,
                          __local float * buf,
                          int n)
{
    size_t idx = get_global_id(0);
    size_t tid = get_local_id(0);
    size_t dim = get_local_size(0);
    size_t gid = get_group_id(0);

    if (idx < n) {
        buf[tid] = g_in[idx];
        int stride = 1;
        while (stride < dim) {
            barrier(CLK_LOCAL_MEM_FENCE);
            int next = tid + stride;
            if (next < dim && gid + stride < n && buf[next] > buf[tid]) {
                float t = buf[next];
                barrier(CLK_LOCAL_MEM_FENCE);
                buf[tid] = t;
            }
            stride = stride << 2;
        }
        if (tid == 0) {
            g_out[gid] = buf[0];
        }
    }
}

// Sorts g_sort indices of g_tri using compare above
// Swaps linearly within work group
// if there was a swap, moves offset by half dimension and reruns
// Does this until no swaps are done
__kernel void sort(__global Tri_g * g_tri,
                   __global uint * g_sort,
                   __global float * height,
                   __global uint * was_swap,
                   __local Tri_g * buf,
                   __local uint * index_buf,
                   float tri_size,
                   int n)
{
    size_t idx = get_global_id(0);
    size_t tid = get_local_id(0);
    size_t dim = get_local_size(0);
    size_t gid = get_group_id(0);

    uint iters = 0;
    uint offset = 0;
    uint idx_o = idx + offset;
    float max = height[0];
    uint swapped = 1;

    while (idx < n && swapped == 1 && iters < 1000) {
        iters++;
        idx_o = idx + offset;

        if (idx_o < n) {
            index_buf[tid] = g_sort[idx_o];
            buf[tid] = g_tri[g_sort[idx_o]];
            barrier(CLK_LOCAL_MEM_FENCE);

            //TODO short cicuit early
            for (int i = 0; i < dim; i++) {
                if (tid + 1 < dim) {
                    bool swap = tri_compare(&buf[tid], &buf[tid + 1],
                                            max, tri_size);
                    if (swap) {
                        swapped = 1;
                        Tri_g t = buf[tid + 1];
                        uint t_i = index_buf[tid + 1];
                        barrier( CLK_LOCAL_MEM_FENCE );
                        buf[tid] = t;
                        index_buf[tid] = t_i;
                    }
                }
                barrier( CLK_LOCAL_MEM_FENCE );
            }

            g_sort[idx_o] = index_buf[tid];
        }

        //set global flag if swapped anything
        if (tid == 0 && swapped == 1) {
            was_swap[0] = 1;
        }

        barrier( CLK_GLOBAL_MEM_FENCE );

        //get global flag
        if (tid == 0) {
            swapped = was_swap[0];
        }

        barrier( CLK_GLOBAL_MEM_FENCE );

        //reset global flag
        if (idx == 0) {
            was_swap[0] = 0;
        }
        barrier( CLK_GLOBAL_MEM_FENCE );

        if (iters % 2 == 1) {
            offset += dim / 2;
        } else {
            offset -= dim / 2;
        }
    }
}

void tri_timeStep(__local Tri_g* tri, float delta) {
    if (tri->inv_mass == 0.0f) {
        return;
    }
    float2 mid = tri_mid_pt(tri);
    float rotation = tri->rot_vel * delta;
    //rotation matrix
    float2 mat1; float2 mat2;
    mat1.x = cos(rotation);
    mat1.y = -sin(rotation);
    mat2.x = sin(rotation);
    mat2.y = cos(rotation);
    for (int i = 0 ; i < 3; i++) {
        float2 rad;
        rad = tri->verts[i] - mid;
        float2 rot_delta;
        rot_delta.x = dot(rad, mat1);
        rot_delta.y = dot(rad, mat2);
        //rotational velocity
        tri->verts[i] = mid + rot_delta;
        //linear velocity
        tri->verts[i] += tri->velocity * delta;
    }

    //acceleration from gravity
    tri->velocity.y += -10.0f * delta;
    tri->rot_vel *= 0.9995f;
    tri->velocity *= 0.9995f;
}

//Timestep all triangles
__kernel void time_step(__global Tri_g * g_tri,
                       __local Tri_g * buf,
                       float delta,
                       int n)
{
    size_t idx = get_global_id(0);
    size_t tid = get_local_id(0);
    size_t dim = get_local_size(0);
    size_t gid = get_group_id(0);

    // if (idx < n) {
    //     buf[tid] = g_tri[idx];
    //     // tri_timeStep(&buf[tid], delta);
    //     g_tri[idx] = buf[tid];
    // }
}
