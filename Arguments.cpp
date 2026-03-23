//
// Created by Pierce Corcoran on 8/31/21.
//

#include "Arguments.h"

#include <iostream>
#include <utility>
#include <exception>
#include <fmt/format.h>

Arguments::Arguments(int index, std::string operation, docopt_map arguments, MachineState *state)
        : index(index), operation(std::move(operation)), arguments(std::move(arguments)), state(state) {}

bool Arguments::has(const std::string &name) const {
    return this->arguments.at(name).kind() != docopt::Kind::Empty;
}

const std::string &Arguments::get_string(const std::string &name) const {
    try {
        return this->arguments.at(name).asString();
    } catch (const std::runtime_error &error) {
        throw std::runtime_error(fmt::format("Error getting value of option '{}': {}", name, error.what()));
    }
}

const std::string &Arguments::get_string(const std::string &name, const std::string &def) const {
    return this->has(name) ? this->get_string(name) : def;
}

bool Arguments::get_bool(const std::string &name) const {
    try {
        return this->arguments.at(name).asBool();
    } catch (const std::runtime_error &error) {
        throw std::runtime_error(fmt::format("Error getting value of option '{}': {}", name, error.what()));
    }
}

int Arguments::get_int(const std::string &name) const {
    if (this->arguments.at(name).isLong()) {
        return (int) this->arguments.at(name).asLong();
    }
    const auto &str = this->get_string(name);
    if (!str.empty() && str[0] == '%') {
        return (int) this->state->eval(str.substr(1));
    }
    return std::stoi(str);
}

int Arguments::get_int(const std::string &name, int def) const {
    return this->has(name) ? this->get_int(name) : def;
}

float Arguments::get_float(const std::string &name) const {
    const auto &str = this->get_string(name);
    if (!str.empty() && str[0] == '%') {
        return (float) this->state->eval(str.substr(1));
    }
    return std::stof(str);
}

float Arguments::get_float(const std::string &name, float def) const {
    return this->has(name) ? this->get_float(name) : def;
}

double Arguments::get_double(const std::string &name) const {
    const auto &str = this->get_string(name);
    if (!str.empty() && str[0] == '%') {
        return (double) this->state->eval(str.substr(1));
    }
    return std::stod(str);
}

double Arguments::get_double(const std::string &name, double def) const {
    return this->has(name) ? this->get_double(name) : def;
}
