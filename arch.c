#include "arch.h"

/*
 * This file exists so that compilation fails immediately on
 * unknown architectures, rather than spewing out dozens of
 * errors as a result of undefined macros.
 *
 * The following declaration suppresses a warning about an empty
 * translation unit.
 */

extern void *dummy;
