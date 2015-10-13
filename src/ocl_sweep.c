
#include "ocl_sweep.h"

void sweep_plane(
    const int octant,
    const unsigned int plane,
    const struct plane * planes,
    struct problem * problem,
    struct context * context,
    struct buffers * buffers
    )
{
    cl_int err;

    size_t global[] = {problem->nang*problem->ng, planes[plane].num_cells};
    err = clEnqueueNDRangeKernel(context->queue,
        context->kernels.sweep_plane,
        2, 0, global, NULL,
        0, NULL, NULL);
    check_ocl(err, "Enqueue plane sweep kernel");
}