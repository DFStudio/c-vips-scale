#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <chrono>
#include <vips/vips.h>
#include <vips/vips8>
#include <docopt/docopt.h>
#include "unsharp.h"

// default --sharpen=0.5/0/10/20/0/3
using namespace vips;

static const char USAGE[] = R"(Vips Scale.
Usage:
  vips-scale --input=<input> --output=<output>
    --width=<width> --height=<height>
    [--quality=<q> --strip]
    [--autorotate --profile=<path> --intent=<intent>]
    [--sharpen=<params>]
    [--stats --unsharp]

Options:
  -h --help           Show this screen.
  --version           Show version.
  --input=<file>      Set the input file.
  --output=<file>     Set the output file.
  --quality=<q>       Set the output quality. [default: 70]
  --strip             Strip metadata in the output.
  --autorotate        Automatically apply rotation from the input file.
  --profile=<path>    The path to the ICC profile.
  --intent=<intent>   The rendering intent. One of:
                      "perceptual", "relative", "saturation", "absolute". [default: relative]
  --sharpen=<params>  The sharpen parameters, if sharpening should be performed.
                      Expressed as slash-delimited numbers: "sigma/x1/y2/y3/m1/m2".
  --stats             Print the time taken to perform each operation.
  --unsharp           Apply an unsharp filter
)";

typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> time_point;

void log_time(bool enabled, time_point &time, const std::string& message) {
    if(!enabled)
        return;
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time);
    std::cout << message << ": " << duration.count() << "ms" << std::endl;
    time = now;
}

int main(int argc, char **argv) {
    std::map<std::string, docopt::value> args = docopt::docopt(
            USAGE,
            { argv + 1, argv + argc },
            true, // show help if requested
            "vips-scale 0.1" // version string
    );

    bool stats = args["--stats"].asBool();

    time_point start = std::chrono::high_resolution_clock::now();
    time_point time = start;

    if(VIPS_INIT( argv[0] ))
        vips_error_exit("init");
    unsharp_get_type();

    log_time(stats, time, "vips init");

    VImage image;

    {
        VipsIntent intent = VIPS_INTENT_RELATIVE;
        if(args["--intent"].isString()) {
            auto arg = args["--intent"].asString();
            if(arg == "perceptual") {
                intent = VIPS_INTENT_PERCEPTUAL;
            } else if(arg == "relative") {
                intent = VIPS_INTENT_RELATIVE;
            } else if(arg == "saturation") {
                intent = VIPS_INTENT_SATURATION;
            } else if(arg == "absolute") {
                intent = VIPS_INTENT_ABSOLUTE;
            } else {
                std::cerr << "Unrecognized intent '" << arg << "'." << std::endl;
                return 1;
            }
        }

        VOption *options = VImage::option()
                ->set("height", (int)args["--height"].asLong())
                ->set("no_rotate", !args["--autorotate"].asBool())
                ->set("intent", intent)
                ;
        if(args["--profile"].isString())
            options = options->set("export_profile", args["--profile"].asString().c_str());

        image = VImage::thumbnail(
                args["--input"].asString().c_str(),
                (int)args["--width"].asLong(),
                options
        );
    }

    log_time(stats, time, "load & thumbnail");

    if(args["--unsharp"].asBool()) {
        VImage blur = image.gaussblur(1.0);
        log_time(stats, time, "blur");
        VipsImage *_sharpened;
        unsharp(image.get_image(), blur.get_image(), &_sharpened);
        image = VImage(_sharpened);

        log_time(stats, time, "unsharp");
    }

    if(args["--sharpen"].isString()) {
        std::stringstream test(args["--sharpen"].asString());
        std::string segment;
        std::vector<float> params;

        while(std::getline(test, segment, '/'))
        {
            params.push_back(std::strtof(segment.c_str(), nullptr));
        }

        if(params.size() != 6) {
            std::cerr << "Invalid sharpen parameters. Expected 6 parameters, got " << params.size() << std::endl;
            return 1;
        }
        image = image.sharpen(
                VImage::option()
                        ->set("sigma", params[0])
                        ->set("x1", params[1])
                        ->set("y2", params[2])
                        ->set("y3", params[3])
                        ->set("m1", params[4])
                        ->set("m2", params[5])
        );

        log_time(stats, time, "sharpen");
    }

    // NOTE: if we wanted to add some extra processing (e.g., calculate an image hash) this would be the place to do it.

    {
        VOption *options = VImage::option()
                ->set("Q", (int)args["--quality"].asLong())
                ->set("strip", args["--strip"].asBool())
                ->set("optimize_coding", true);
        image.write_to_file(args["--output"].asString().c_str(), options);

        log_time(stats, time, "write");
    }

    log_time(stats, start, "total");

    return( 0 );
}