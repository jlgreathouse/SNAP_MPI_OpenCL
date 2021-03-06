
#include <stdlib.h>

#include "problem.h"
#include "allocate.h"

void allocate_memory(struct problem * problem, struct rankinfo * rankinfo, struct memory * memory)
{
    // Allocate two copies of the angular flux
    // grid * angles * noct (8) * ng
    memory->angular_flux_in = malloc(sizeof(double)*rankinfo->nx*rankinfo->ny*rankinfo->nz*problem->nang*8*problem->ng);
    memory->angular_flux_out = malloc(sizeof(double)*rankinfo->nx*rankinfo->ny*rankinfo->nz*problem->nang*8*problem->ng);

    // Allocate edge arrays
    memory->flux_i = malloc(sizeof(double)*problem->nang*problem->ng*rankinfo->ny*rankinfo->nz);
    memory->flux_j = malloc(sizeof(double)*problem->nang*problem->ng*rankinfo->nx*rankinfo->nz);
    memory->flux_k = malloc(sizeof(double)*problem->nang*problem->ng*rankinfo->nx*rankinfo->ny);

    // Scalar flux
    // grid * ng
    memory->scalar_flux = malloc(sizeof(double)*rankinfo->nx*rankinfo->ny*rankinfo->nz*problem->ng);
    memory->old_inner_scalar_flux = malloc(sizeof(double)*rankinfo->nx*rankinfo->ny*rankinfo->nz*problem->ng);
    memory->old_outer_scalar_flux = malloc(sizeof(double)*rankinfo->nx*rankinfo->ny*rankinfo->nz*problem->ng);

    //Scalar flux moments
    if (problem->cmom-1 == 0)
        memory->scalar_flux_moments = NULL;
    else
        memory->scalar_flux_moments = malloc(sizeof(double)*(problem->cmom-1)*problem->ng*rankinfo->nx*rankinfo->ny*rankinfo->nz);

    // Cosine coefficients
    memory->mu = malloc(sizeof(double)*problem->nang);
    memory->eta = malloc(sizeof(double)*problem->nang);
    memory->xi = malloc(sizeof(double)*problem->nang);

    // Material cross section
    memory->mat_cross_section = malloc(sizeof(double)*problem->ng);

}

void free_memory(struct memory * memory)
{
    free(memory->angular_flux_in);
    free(memory->angular_flux_out);
    free(memory->flux_i);
    free(memory->flux_j);
    free(memory->flux_k);
    free(memory->scalar_flux);
    free(memory->old_inner_scalar_flux);
    free(memory->old_outer_scalar_flux);
    if (memory->scalar_flux_moments)
        free(memory->scalar_flux_moments);
    free(memory->mu);
    free(memory->eta);
    free(memory->xi);
    free(memory->mat_cross_section);
}
