
#include "comms.h"

void check_mpi(const int err, const char *msg)
{
    if (err != MPI_SUCCESS)
    {
        fprintf(stderr, "MPI Error: %d. %s\n", err, msg);
        exit(err);
    }
}


void setup_comms(struct problem * problem, struct rankinfo * rankinfo)
{
    // Create the MPI Cartesian topology
    int dims[] = {problem->npex, problem->npey, problem->npez};
    int periods[] = {0, 0, 0};
    int mpi_err = MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, 0, &snap_comms);
    check_mpi(mpi_err, "Creating MPI Cart");

    // Get my ranks in x, y and z
    mpi_err = MPI_Comm_rank(MPI_COMM_WORLD, &rankinfo->rank);
    check_mpi(mpi_err, "Getting MPI rank");
    mpi_err = MPI_Cart_coords(snap_comms, rankinfo->rank, 3, rankinfo->ranks);
    check_mpi(mpi_err, "Getting Cart co-ordinates");

    // Note: The following assumes one tile per MPI rank
    // TODO: Change to allow for tiling

    // Calculate rankinfo sizes
    rankinfo->nx = problem->nx / problem->npex;
    rankinfo->ny = problem->ny / problem->npey;
    rankinfo->nz = problem->nz / problem->npez;

    // Calculate i,j,k lower and upper bounds in terms of problem grid
    rankinfo->ilb = rankinfo->ranks[0]     * rankinfo->nx;
    rankinfo->iub = (rankinfo->ranks[0]+1) * rankinfo->nx;
    rankinfo->jlb = rankinfo->ranks[1]     * rankinfo->ny;
    rankinfo->jub = (rankinfo->ranks[1]+1) * rankinfo->ny;
    rankinfo->klb = rankinfo->ranks[2]     * rankinfo->nz;
    rankinfo->kub = (rankinfo->ranks[2]+1) * rankinfo->nz;

    // Calculate neighbouring ranks
    calculate_neighbours(snap_comms, problem, rankinfo);
}

void finish_comms(void)
{
    int mpi_err = MPI_Finalize();
    check_mpi(mpi_err, "MPI_Finalize");
}

void calculate_neighbours(MPI_Comm comms,  struct problem * problem, struct rankinfo * rankinfo)
{
    int mpi_err;

    // Calculate my neighbours
    int coords[3];
    // x-dir + 1
    coords[0] = (rankinfo->ranks[0] == problem->npex - 1) ? rankinfo->ranks[0] : rankinfo->ranks[0] + 1;
    coords[1] = rankinfo->ranks[1];
    coords[2] = rankinfo->ranks[2];
    mpi_err = MPI_Cart_rank(comms, coords, &rankinfo->xup);
    check_mpi(mpi_err, "Getting x+1 rank");
    // x-dir - 1
    coords[0] = (rankinfo->ranks[0] == 0) ? rankinfo->ranks[0] : rankinfo->ranks[0] - 1;
    coords[1] = rankinfo->ranks[1];
    coords[2] = rankinfo->ranks[2];
    mpi_err = MPI_Cart_rank(comms, coords, &rankinfo->xdown);
    check_mpi(mpi_err, "Getting x-1 rank");
    // y-dir + 1
    coords[0] = rankinfo->ranks[0];
    coords[1] = (rankinfo->ranks[1] == problem->npey - 1) ? rankinfo->ranks[1] : rankinfo->ranks[1] + 1;
    coords[2] = rankinfo->ranks[2];
    mpi_err = MPI_Cart_rank(comms, coords, &rankinfo->yup);
    check_mpi(mpi_err, "Getting y+1 rank");
    // y-dir - 1
    coords[0] = rankinfo->ranks[0];
    coords[1] = (rankinfo->ranks[1] == 0) ? rankinfo->ranks[1] : rankinfo->ranks[1] - 1;
    coords[2] = rankinfo->ranks[2];
    mpi_err = MPI_Cart_rank(comms, coords, &rankinfo->ydown);
    check_mpi(mpi_err, "Getting y-1 rank");
    // z-dir + 1
    coords[0] = rankinfo->ranks[0];
    coords[1] = rankinfo->ranks[1];
    coords[2] = (rankinfo->ranks[2] == problem->npez - 1) ? rankinfo->ranks[2] : rankinfo->ranks[2] + 1;
    mpi_err = MPI_Cart_rank(comms, coords, &rankinfo->zup);
    check_mpi(mpi_err, "Getting z+1 rank");
    // z-dir - 1
    coords[0] = rankinfo->ranks[0];
    coords[1] = rankinfo->ranks[1];
    coords[2] = (rankinfo->ranks[2] == 0) ? rankinfo->ranks[2] : rankinfo->ranks[2] - 1;
    mpi_err = MPI_Cart_rank(comms, coords, &rankinfo->zdown);
    check_mpi(mpi_err, "Getting z-1 rank");
}


void recv_boundaries(const int octant, const int istep, const int jstep, const int kstep,
    struct problem * problem, struct rankinfo * rankinfo,
    struct memory * memory, struct context * context, struct buffers * buffers)
{
    int mpi_err;
    cl_int cl_err;

    // Check if pencil has an external boundary for this sweep direction
    // If so, set as vacuum
    if ( (istep == -1 && rankinfo->iub == problem->nx)
        || (istep == 1 && rankinfo->ilb == 0))
    {
        zero_buffer(context, buffers->flux_i, problem->nang*problem->ng*rankinfo->ny*rankinfo->nz);
    }
    // Otherwise, internal boundary - get data from MPI receives
    else
    {
        if (istep == -1)
        {
            mpi_err = MPI_Recv(memory->flux_i, problem->nang*problem->ng*rankinfo->ny*rankinfo->nz, MPI_DOUBLE,
                rankinfo->xup, MPI_ANY_TAG, snap_comms, NULL);
            check_mpi(mpi_err, "Receiving from upward x neighbour");
        }
        else
        {
            mpi_err = MPI_Recv(memory->flux_i, problem->nang*problem->ng*rankinfo->ny*rankinfo->nz, MPI_DOUBLE,
                rankinfo->xdown, MPI_ANY_TAG, snap_comms, NULL);
            check_mpi(mpi_err, "Receiving from downward x neighbour");
        }
        // Copy flux_i to the device
        cl_err = clEnqueueWriteBuffer(context->queue, buffers->flux_i, CL_TRUE, 0,
            sizeof(double)*problem->nang*problem->ng*rankinfo->ny*rankinfo->nz, memory->flux_i,
            0, NULL, NULL);
        check_ocl(cl_err, "Copying flux i buffer to device");
    }

    if ( (jstep == -1 && rankinfo->jub == problem->ny)
        || (jstep == 1 && rankinfo->jlb == 0))
    {
        zero_buffer(context, buffers->flux_j, problem->nang*problem->ng*rankinfo->nx*rankinfo->nz);
    }
    else
    {
        if (jstep == -1)
        {
            mpi_err = MPI_Recv(memory->flux_j, problem->nang*problem->ng*rankinfo->nx*rankinfo->nz, MPI_DOUBLE,
                rankinfo->yup, MPI_ANY_TAG, snap_comms, NULL);
            check_mpi(mpi_err, "Receiving from upward y neighbour");
        }
        else
        {
            mpi_err = MPI_Recv(memory->flux_j, problem->nang*problem->ng*rankinfo->nx*rankinfo->nz, MPI_DOUBLE,
                rankinfo->ydown, MPI_ANY_TAG, snap_comms, NULL);
            check_mpi(mpi_err, "Receiving from downward y neighbour");
        }
        // Copy flux_i to the device
        cl_err = clEnqueueWriteBuffer(context->queue, buffers->flux_j, CL_TRUE, 0,
            sizeof(double)*problem->nang*problem->ng*rankinfo->nx*rankinfo->nz, memory->flux_j,
            0, NULL, NULL);
        check_ocl(cl_err, "Copying flux j buffer to device");
    }

    if ( (kstep == -1 && rankinfo->kub == problem->nz)
        || (kstep == 1 && rankinfo->klb == 0))
    {
        zero_buffer(context, buffers->flux_k, problem->nang*problem->ng*rankinfo->nx*rankinfo->ny);
    }
    else
    {
        if (kstep == -1)
        {
            mpi_err = MPI_Recv(memory->flux_k, problem->nang*problem->ng*rankinfo->nx*rankinfo->ny, MPI_DOUBLE,
                rankinfo->zup, MPI_ANY_TAG, snap_comms, NULL);
            check_mpi(mpi_err, "Receiving from upward z neighbour");
        }
        else
        {
            mpi_err = MPI_Recv(memory->flux_k, problem->nang*problem->ng*rankinfo->nx*rankinfo->ny, MPI_DOUBLE,
                rankinfo->zdown, MPI_ANY_TAG, snap_comms, NULL);
            check_mpi(mpi_err, "Receiving from downward z neighbour");
        }
        // Copy flux_i to the device
        cl_err = clEnqueueWriteBuffer(context->queue, buffers->flux_k, CL_TRUE, 0,
            sizeof(double)*problem->nang*problem->ng*rankinfo->nx*rankinfo->ny, memory->flux_k,
            0, NULL, NULL);
        check_ocl(cl_err, "Copying flux k buffer to device");
    }
}


void send_boundaries(const int octant, const int istep, const int jstep, const int kstep,
    struct problem * problem, struct rankinfo * rankinfo,
    struct memory * memory, struct context * context, struct buffers * buffers)
{
    int mpi_err;
    cl_int cl_err;

    // Get the edges off the device
    cl_err = clEnqueueReadBuffer(context->queue, buffers->flux_i, CL_FALSE,
        0, sizeof(double)*problem->nang*problem->ng*rankinfo->ny*rankinfo->nz, memory->flux_i, 0, NULL, NULL);
    check_ocl(cl_err, "Copying flux i buffer back to host");
    cl_err = clEnqueueReadBuffer(context->queue, buffers->flux_j, CL_FALSE,
        0, sizeof(double)*problem->nang*problem->ng*rankinfo->nx*rankinfo->nz, memory->flux_j, 0, NULL, NULL);
    check_ocl(cl_err, "Copying flux j buffer back to host");
    cl_err = clEnqueueReadBuffer(context->queue, buffers->flux_k, CL_TRUE,
        0, sizeof(double)*problem->nang*problem->ng*rankinfo->nx*rankinfo->ny, memory->flux_k, 0, NULL, NULL);
    check_ocl(cl_err, "Copying flux k buffer back to host");

    // Send to neighbour with MPI_Send
    // X
    if (istep == -1 && rankinfo->xdown != rankinfo->rank)
    {
        mpi_err = MPI_Send(memory->flux_i, problem->nang*problem->ng*rankinfo->ny*rankinfo->nz, MPI_DOUBLE,
                rankinfo->xdown, 0, snap_comms);
        check_mpi(mpi_err, "Sending to downward x neighbour");
    }
    else if (istep == 1 && rankinfo->xup != rankinfo->rank)
    {
        mpi_err = MPI_Send(memory->flux_i, problem->nang*problem->ng*rankinfo->ny*rankinfo->nz, MPI_DOUBLE,
                rankinfo->xup, 0, snap_comms);
        check_mpi(mpi_err, "Sending to upward x neighbour");
    }
    // Y
    if (jstep == -1 && rankinfo->ydown != rankinfo->rank)
    {
        mpi_err = MPI_Send(memory->flux_j, problem->nang*problem->ng*rankinfo->nx*rankinfo->nz, MPI_DOUBLE,
                rankinfo->ydown, 0, snap_comms);
        check_mpi(mpi_err, "Sending to downward y neighbour");
    }
    else if (jstep == 1 && rankinfo->yup != rankinfo->rank)
    {
        mpi_err = MPI_Send(memory->flux_j, problem->nang*problem->ng*rankinfo->nx*rankinfo->nz, MPI_DOUBLE,
                rankinfo->yup, 0, snap_comms);
        check_mpi(mpi_err, "Sending to upward y neighbour");
    }
    // Z
    if (kstep == -1 && rankinfo->zdown != rankinfo->rank)
    {
        mpi_err = MPI_Send(memory->flux_k, problem->nang*problem->ng*rankinfo->nx*rankinfo->ny, MPI_DOUBLE,
                rankinfo->zdown, 0, snap_comms);
        check_mpi(mpi_err, "Sending to downward z neighbour");
    }
    else if (kstep == 1 && rankinfo->zup != rankinfo->rank)
    {
        mpi_err = MPI_Send(memory->flux_k, problem->nang*problem->ng*rankinfo->nx*rankinfo->ny, MPI_DOUBLE,
                rankinfo->zup, 0, snap_comms);
        check_mpi(mpi_err, "Sending to upward z neighbour");
    }
}


