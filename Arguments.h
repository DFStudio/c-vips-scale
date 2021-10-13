//
// Created by Pierce Corcoran on 8/31/21.
//

#ifndef VIPS_SCALE_ARGUMENTS_H
#define VIPS_SCALE_ARGUMENTS_H

#include <string>
#include <vector>
#include "MachineState.h"

class Arguments {
    std::vector<std::string> arguments;
    MachineState *state;
public:
    const int index;
    const std::string name;

    explicit Arguments(int index, std::string name, std::vector<std::string> arguments, MachineState *state);

    void require(size_t count) const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool has(size_t index) const;
    [[nodiscard]] const std::string& get_string(size_t index) const;
    [[nodiscard]] bool get_bool(size_t index) const;
    [[nodiscard]] int get_int(size_t index) const;
    [[nodiscard]] float get_float(size_t index) const;
    [[nodiscard]] double get_double(size_t index) const;

    [[nodiscard]] const std::vector<std::string> &get_arguments() const;
};


#endif //VIPS_SCALE_ARGUMENTS_H