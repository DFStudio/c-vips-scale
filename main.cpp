#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <vips/vips.h>
#include <vips/vips8>
#include "unsharp.h"
#include "Arguments.h"
#include "operations.h"
#include "MachineState.h"

using namespace vips;

int main(int argc, char **argv) {
    if (VIPS_INIT(argv[0]))
        vips_error_exit("init");
    unsharp_get_type();

    MachineState state;
    initialize_functions(&state);

    std::vector<Arguments> commands;
    std::string current_name;
    std::vector<std::string> current_command;

    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (!arg.empty() && arg[0] == '@') {
            if (!current_name.empty())
                commands.emplace_back(current_name, current_command, &state);
            current_name = arg.substr(1);
            current_command.clear();
        } else {
            current_command.push_back(arg);
        }
    }

    if (!current_name.empty())
        commands.emplace_back(current_name, current_command, &state);

    for (auto &command: commands) {
        auto operation = get_operation(command.name);
        operation(&state, command);
    }

    return (0);
}