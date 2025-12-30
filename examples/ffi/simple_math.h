#ifndef SIMPLE_MATH_H
#define SIMPLE_MATH_H

int add(int a, int b);
double average(const double *values, unsigned int count);
void fill_buffer(char *buffer, unsigned int len);

struct point {
    double x;
    double y;
};

#endif // SIMPLE_MATH_H
