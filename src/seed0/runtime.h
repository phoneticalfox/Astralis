#pragma once
#include "value.h"

// output
void rt_show(const Value* v);
void rt_warn(const Value* v);

// input
Value rt_ask(const Value* prompt);
