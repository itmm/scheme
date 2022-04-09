/**
 * convert a text stream to a Scheme expression
 */

#pragma once

#include "obj.h"
#include <sstream>

int get(std::istream &in);
double float_value(const std::string &v);

Obj *read_expression(std::istream &in);
Obj *parse_expression(std::istream &in);
Obj *parse_expression(const std::string &in);
