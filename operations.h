//
// Created by Pierce Corcoran on 8/31/21.
//

#ifndef VIPS_SCALE_OPERATIONS_H
#define VIPS_SCALE_OPERATIONS_H

#include <map>
#include <vector>
#include <vips/vips.h>
#include <vips/vips8>
#include "Arguments.h"

/**
 * Returns true if the operation was successful. Returning false aborts.
 */
typedef void (*image_operation)(std::map<int, vips::VImage> &slots, const Arguments &arguments);

image_operation get_operation(const std::string &name);

#endif //VIPS_SCALE_OPERATIONS_H
