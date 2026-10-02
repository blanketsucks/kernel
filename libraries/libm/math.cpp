#include <math.h>

extern "C" {

double fabs(double x) {
    return __builtin_fabs(x);
}

}