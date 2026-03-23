//
// Created by Pierce Corcoran on 9/14/21.
//

#include "MachineState.h"

#include <exception>
#include <utility>
#include <fmt/format.h>
#include <sys/fcntl.h>

void MachineState::set_image(const std::string &name, vips::VImage image) {
    this->slots[name] = std::move(image);
}

vips::VImage MachineState::get_image(const std::string &name) {
    auto image = this->slots.find(name);
    if(image == this->slots.end())
        throw std::invalid_argument(fmt::format("No image in slot {}", name));
    return image->second;
}

void MachineState::free_image(const std::string &name) {
    this->slots.erase(name);
}

void MachineState::set_variable(const std::string &name, double value) {
    auto existing = this->variable_store.find(name);
    double *value_store;
    if(existing != this->variable_store.end()) {
        value_store = existing->second;
    } else {
        value_store = new double;
        this->variable_store[name] = value_store;
    }
    *value_store = value;
    this->symbols.add_variable(name, *value_store, true);
}

double MachineState::get_variable(const std::string &name) {
    return this->symbols.get_variable(name)->value();
}

double MachineState::eval(const std::string &expr) {
    exprtk::parser<double> parser;
    auto expression = parser.compile(expr, this->symbols);
    if(parser.error_count() > 0) {
        std::string error = fmt::format("Error(s) parsing '{}': ", expr);
        for(int i = 0; i < parser.error_count(); i++) {
            if(i != 0)
                error += ", ";
            error += parser.get_error(i).diagnostic;
        }
        throw std::runtime_error(error);
    }
    return expression.value();
}

void MachineState::set_verbosity(int level) {
    this->_verbosity = level;
}

int MachineState::verbosity() const {
    return this->_verbosity;
}

bool MachineState::is_verbose(int level) const {
    return this->verbosity() >= level;
}

MachineState::MachineState() = default;

MachineState::~MachineState() {
    for(auto &[name, reference] : this->variable_store) {
        delete reference;
    }
    for(auto &[name, function] : this->functions) {
        delete function;
    }
}

void MachineState::add_function(const std::string &name, double_function *function) {
    auto existing = this->functions.find(name);
    if(existing != this->functions.end()) {
        this->symbols.remove_function(name);
        delete existing->second;
    }
    this->functions[name] = function;
    this->symbols.add_function(name, *function);
}

void MachineState::open_fd(const std::string &name, const std::string &file, bool input) {
    auto existing = this->files.find(name);
    if(existing != this->files.end() && !existing->second.used) {
        close(existing->second.fd);
    }

    this->files[name] = {
            open(file.c_str(), input ? O_RDONLY : O_WRONLY),
            input,
            false
    };
}

int MachineState::parse_fd(const std::string &arg, bool input) {
    if(arg == "-") {
        return input ? 0 : 1; // stdin : stdout
    } else if(arg.size() >= 7 && arg.substr(0, 7) == "stream:") {
        auto entry = this->files.find(arg.substr(7));
        if(entry == this->files.end()) {
            throw std::invalid_argument(fmt::format("Unknown stream {}", arg.substr(3)));
        } else if(entry->second.used) {
            throw std::invalid_argument(fmt::format(
                    "Stream {} has already been used. Reopen it to use it again",
                    arg.substr(3)
            ));
        } else if(entry->second.input != input) {
            throw std::invalid_argument(fmt::format(
                    "Stream {} is opened for {} but is being used for {}",
                    arg.substr(3),
                    entry->second.input ? "input" : "output",
                    input ? "input" : "output"
            ));
        } else {
            entry->second.used = true;
            return entry->second.fd;
        }
    } else {
        return -1;
    }
}
