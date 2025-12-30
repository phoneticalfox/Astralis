#ifndef SIMPLE_MATH_H
#define SIMPLE_MATH_H

#include <stdbool.h>
#include <stddef.h>

typedef unsigned long count_t;

enum math_error {
    MATH_OK = 0,
    MATH_DIV_ZERO = 1,
};

struct point {
    double x;
    double y;
};

union value_box {
    int i;
    double d;
};

extern const double PI;
extern count_t global_calls;

int add(int a, int b);
double average(const double *values, unsigned int count);
void fill_buffer(char *buffer, unsigned int len);
bool midpoint(const struct point *a, const struct point *b, struct point *out);
union value_box to_value_box(double input);

#endif // SIMPLE_MATH_H
