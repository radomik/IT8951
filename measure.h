#ifndef _MEASURE_H_
#define _MEASURE_H_

#include <time.h>

#define meas_start() \
 clock_t t_start = clock()
 
#define meas_end() \
 fprintf(stderr, "%s # %s() #%d -> Elapsed %.3f\n", __FILE__, __FUNCTION__, __LINE__, ((double) (clock() - t_start)) / CLOCKS_PER_SEC)

#endif
