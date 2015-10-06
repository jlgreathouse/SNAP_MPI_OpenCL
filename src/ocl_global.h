
#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


/** \file
* \brief OpenCL routines
*/

/**
\brief Structure to contain OpenCL context, command queue, device and program objects
*/
struct context
{
    cl_platform_id platform;
    cl_context context;
    cl_device_id device;
    cl_command_queue queue;
    cl_program program;
};

/** \brief Initilise the OpenCL device, context, command queue and program */
void init_ocl(struct context * context);

/** \brief Release the OpenCL objects held in the context structure */
void release_context(struct context * context);