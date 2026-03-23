#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"
//
// Created by Pierce Corcoran on 8/31/21.
//

#include "operations.h"
#include <limits>
#include <exception>
#include <fmt/format.h>
#include "unsharp.h"
#include "phash.h"

using namespace vips;

std::vector<double> trim_bands(int bands, std::vector<double> components) {
    std::vector<double> out;
    for (int i = 0; i < bands && i < components.size(); i++) {
        out.push_back(components[i]);
    }
    return out;
}

VipsInteresting parse_interest(const std::string &arg) {
    if (arg == "none") return VIPS_INTERESTING_NONE;
    if (arg == "centre") return VIPS_INTERESTING_CENTRE;
    if (arg == "center") return VIPS_INTERESTING_CENTRE;
    if (arg == "entropy") return VIPS_INTERESTING_ENTROPY;
    if (arg == "attention") return VIPS_INTERESTING_ATTENTION;
    if (arg == "low") return VIPS_INTERESTING_LOW;
    if (arg == "high") return VIPS_INTERESTING_HIGH;
    if (arg == "all") return VIPS_INTERESTING_ALL;
    throw std::invalid_argument(fmt::format("Unrecognized interest '{}'", arg));
}

VipsIntent parse_intent(const std::string &arg) {
    if (arg == "perceptual") return VIPS_INTENT_PERCEPTUAL;
    if (arg == "relative") return VIPS_INTENT_RELATIVE;
    if (arg == "saturation") return VIPS_INTENT_SATURATION;
    if (arg == "absolute") return VIPS_INTENT_ABSOLUTE;
    throw std::invalid_argument(fmt::format("Unrecognized intent '{}'", arg));
}

VipsSize parse_size(const std::string &arg) {
    if (arg == "both") return VIPS_SIZE_BOTH;
    if (arg == "up") return VIPS_SIZE_UP;
    if (arg == "down") return VIPS_SIZE_DOWN;
    if (arg == "force") return VIPS_SIZE_FORCE;
    throw std::invalid_argument(fmt::format("Unrecognized size type '{}'", arg));
}

VipsBlendMode parse_blend_mode(const std::string &arg) {
    if (arg == "clear") return VIPS_BLEND_MODE_CLEAR;
    if (arg == "source") return VIPS_BLEND_MODE_SOURCE;
    if (arg == "over") return VIPS_BLEND_MODE_OVER;
    if (arg == "in") return VIPS_BLEND_MODE_IN;
    if (arg == "out") return VIPS_BLEND_MODE_OUT;
    if (arg == "atop") return VIPS_BLEND_MODE_ATOP;
    if (arg == "dest") return VIPS_BLEND_MODE_DEST;
    if (arg == "dest_over") return VIPS_BLEND_MODE_DEST_OVER;
    if (arg == "dest_in") return VIPS_BLEND_MODE_DEST_IN;
    if (arg == "dest_out") return VIPS_BLEND_MODE_DEST_OUT;
    if (arg == "dest_atop") return VIPS_BLEND_MODE_DEST_ATOP;
    if (arg == "xor") return VIPS_BLEND_MODE_XOR;
    if (arg == "add") return VIPS_BLEND_MODE_ADD;
    if (arg == "saturate") return VIPS_BLEND_MODE_SATURATE;
    if (arg == "multiply") return VIPS_BLEND_MODE_MULTIPLY;
    if (arg == "screen") return VIPS_BLEND_MODE_SCREEN;
    if (arg == "overlay") return VIPS_BLEND_MODE_OVERLAY;
    if (arg == "darken") return VIPS_BLEND_MODE_DARKEN;
    if (arg == "lighten") return VIPS_BLEND_MODE_LIGHTEN;
    if (arg == "colour_dodge") return VIPS_BLEND_MODE_COLOUR_DODGE;
    if (arg == "colour_burn") return VIPS_BLEND_MODE_COLOUR_BURN;
    if (arg == "hard_light") return VIPS_BLEND_MODE_HARD_LIGHT;
    if (arg == "soft_light") return VIPS_BLEND_MODE_SOFT_LIGHT;
    if (arg == "difference") return VIPS_BLEND_MODE_DIFFERENCE;
    if (arg == "exclusion") return VIPS_BLEND_MODE_EXCLUSION;
    throw std::invalid_argument(fmt::format("Unrecognized blend mode '{}'", arg));
}

const Operation cmd_load{
        "load",
        R"(
Load an image from a file or stream

Usage:
  @load <file> <slot>
)",
        [](MachineState *state, const Arguments &arguments) {
            int fd = state->parse_fd(arguments.get_string("<file>"), true);
            if (fd != -1) {
                state->set_image(
                        arguments.get_string("<slot>"),
                        VImage::new_from_source(VSource::new_from_descriptor(fd), "")
                );
            } else {
                state->set_image(
                        arguments.get_string("<slot>"),
                        VImage::new_from_file(arguments.get_string("<file>").c_str())
                );
            }
        }
};

const Operation cmd_open_stream{
        "open_stream",
        R"(
Open a file for streaming. In future operations it can be referred to as 'stream:<name>'.
Streams can only be used once this way and will have to be reopened to be used again.

Usage:
  @open_stream <name> <file> [--input]

Options:
  -i --input  Open the stream for reading
)",
        [](MachineState *state, const Arguments &arguments) {
            state->open_fd(
                    arguments.get_string("<name>"),
                    arguments.get_string("<file>"),
                    arguments.get_bool("--input")
            );
        }
};
const Operation cmd_thumbnail{
        "thumbnail",
        R"(
Create a thumbnail from a file or stream

Usage:
  @thumbnail <file> <slot> <width> [<height>] [--no-rotate] [--intent=<intent>] [--sizing=<sizing>] [--crop=<crop>]

Options:
  --no-rotate  Don't automatically flatten rotation metadata
  --intent=<intent>  The VIPS scaling intent. One of 'perceptual',
                     'relative', 'saturation', 'absolute'. [default: relative]
  --sizing=<sizing>  The VIPS sizing type. One of 'both', 'up', 'down', 'force'.
                     [default: down]
  --crop=<crop>      Scale to fill, rather than fit, using the given VIPS
                     "interest" strategy. One of 'none', 'centre', 'entropy',
                     'attention', 'low', 'high'. If set to a non-'none' value
                     both <width> and <height> must be specified. [default: none]
)",
        [](MachineState *state, const Arguments &arguments) {
            VipsIntent intent = parse_intent(arguments.get_string("--intent"));
            VipsInteresting crop = parse_interest(arguments.get_string("--crop"));

            VipsSize size = parse_size(arguments.get_string("--sizing"));

            VOption *voptions = VImage::option()
                    ->set("no_rotate", arguments.get_bool("--no-rotate"))
                    ->set("intent", intent)
                    ->set("crop", crop)
                    ->set("size", size);
            if (arguments.has("<height>"))
                voptions->set("height", arguments.get_int("<height>"));

            int fd = state->parse_fd(arguments.get_string("<file>"), true);
            if (fd != -1) {
                state->set_image(arguments.get_string("<slot>"), VImage::thumbnail_source(
                        VSource::new_from_descriptor(fd),
                        arguments.get_int("<width>"),
                        voptions
                ));
            } else {
                state->set_image(arguments.get_string("<slot>"), VImage::thumbnail(
                        arguments.get_string("<file>").c_str(),
                        arguments.get_int("<width>"),
                        voptions
                ));
            }
        }
};

const Operation cmd_autorotate{
        "autorotate",
        R"(
Flatten rotation metadata on an image

Usage:
  @autorotate <slot_in> <slot_out>
)",
        [](MachineState *state, const Arguments &arguments) {
            state->set_image(
                    arguments.get_string("<slot_out>"),
                    state->get_image(arguments.get_string("<slot_in>")).copy().autorot()
            );
        }

};

const Operation cmd_profile{
        "profile",
        R"(
Apply an ICC color profile

Usage:
  @profile <slot_in> <slot_out> --profile=<profile>

Options:
  --profile=<profile>  The output ICC color profile
)",
        [](MachineState *state, const Arguments &arguments) {
            state->set_image(
                    arguments.get_string("<slot_out>"),
                    state->get_image(arguments.get_string("<slot_in>"))
                            .icc_transform(arguments.get_string("--profile").c_str())
            );
        }
};

const Operation cmd_unsharp{
        "unsharp",
        R"(
Perform an "unsharp" operation on an image

Usage:
  @unsharp <slot_in> <slot_out> <sigma> <strength>
)",
        [](MachineState *state, const Arguments &arguments) {
            auto input = state->get_image(arguments.get_string("<slot_in>"));
            VipsImage *sharpened;
            unsharp(
                    input.get_image(), &sharpened,
                    "sigma", arguments.get_double("<sigma>"),
                    "strength", arguments.get_double("<strength>"),
                    NULL
            );
            state->set_image(arguments.get_string("<slot_out>"), VImage(sharpened));
        }
};

const Operation cmd_composite{
        "composite",
        R"(
Composite two images

Usage:
  @composite <base_slot> <overlay_slot> <slot_out> <overlay_x> <overlay_y> [--blend-mode=<mode>]

Options:
  --blend-mode=<mode>  The blending mode to use for the composite operation
                       (see 'vips-tool help enums') [default: over]
)",
        [](MachineState *state, const Arguments &arguments) {
            // <base slot = 0> <overlay slot = 1> <slot out = 2> <overlay x = 3> <overlay y = 4> <blend mode? = 5>
            state->set_image(
                    arguments.get_string("<slot_out>"),
                    state->get_image(arguments.get_string("<base_slot>"))
                            .composite2(
                                    state->get_image(arguments.get_string("<overlay_slot>")),
                                    parse_blend_mode(arguments.get_string("--blend-mode")),
                                    VImage::option()
                                            ->set("x", arguments.get_int("<overlay_x>"))
                                            ->set("y", arguments.get_int("<overlay_y>"))
                            )
            );
        }
};

struct vec2d {
    float x, y;

    vec2d operator+(const vec2d &other) const { return {this->x + other.x, this->y + other.y}; }

    vec2d operator-(const vec2d &other) const { return {this->x - other.x, this->y - other.y}; }

    vec2d operator*(const vec2d &other) const { return {this->x * other.x, this->y * other.y}; }

    vec2d operator*(float other) const { return {this->x * other, this->y * other}; }

    vec2d operator/(const vec2d &other) const { return {this->x / other.x, this->y / other.y}; }

    vec2d operator/(float other) const { return {this->x / other, this->y / other}; }

    vec2d operator-() const { return {-this->x, -this->y}; }
};

int count_to_bound(float base_size, float item_size, float origin, float step) {
    if (step == 0) {
        return 0;
    } else if (step < 0) {
        return (int) floor((origin + item_size) / -step);
    } else {
        return (int) floor((base_size - origin) / step);
    }
}

int count_to_bound(vec2d base_size, vec2d item_size, vec2d origin, vec2d step, bool max) {
    return max ? std::max(
            step.x == 0 ? 0 : count_to_bound(base_size.x, item_size.x, origin.x, step.x),
            step.y == 0 ? 0 : count_to_bound(base_size.y, item_size.y, origin.y, step.y)
    ) : std::min(
            step.x == 0 ? std::numeric_limits<int>::max() : count_to_bound(base_size.x, item_size.x, origin.x,
                                                                           step.x),
            step.y == 0 ? std::numeric_limits<int>::max() : count_to_bound(base_size.y, item_size.y, origin.y,
                                                                           step.y)
    );
}

const Operation cmd_grid{
        "grid",
        R"(
Generate an image by repeating an image in an arbitrary grid.

Usage:
  @grid <base_slot> <overlay_slot> <slot_out> [--origin-x=<x>] [--origin-y=<y>]
        [--vstep-x=<x> --vstep-y=<y>] [--hstep-x=<x> --hstep-y=<y>]
        [--blend-mode=<mode>]

Options:
  --origin-x=<x>       The X coordinate for the "0,0" image [default: 0]
  --origin-y=<y>       The Y coordinate for the "0,0" image [default: 0]
  --vstep-x=<x>        The X step for the "vertical" axis (defaults to 0)
  --vstep-y=<y>        The Y step for the "vertical" axis (defaults to the overlay height)
  --hstep-x=<x>        The X step for the "horizontal" axis (defaults to the overlay width)
  --hstep-y=<y>        The Y step for the "horizontal" axis (defaults to 0)
  --blend-mode=<mode>  The blend mode to use for the composite operations.
                       (see 'vips-tool help enums') [default: over]
)",
        [](MachineState *state, const Arguments &arguments) {
            // <base slot = 0> <overlay slot = 1> <slot out = 2> <origin x = 3> <origin y = 4>
            //   <vertical step x = 5> <vertical step y = 6> <horizontal step x = 7> <horizontal step y = 8>
            //   <blend mode? = 9>

            VipsBlendMode mode = parse_blend_mode(arguments.get_string("--blend-mode"));

            auto base = state->get_image(arguments.get_string("<base_slot>"));
            auto watermark = state->get_image(arguments.get_string("<overlay_slot>"));

            vec2d origin{arguments.get_float("--origin-x"),
                         arguments.get_float("--origin-y")};
            vec2d vStep{arguments.get_float("--vstep-x", 0),
                        arguments.get_float("--vstep-y", (float) watermark.height())};
            vec2d hStep{arguments.get_float("--hstep-x", (float) watermark.width()),
                        arguments.get_float("--hstep-y", 0)};
            vec2d base_size{(float) base.width(), (float) base.height()};
            vec2d item_size{(float) watermark.width(), (float) watermark.height()};

            int hMin = -count_to_bound(base_size, item_size, origin, -hStep, true);
            int hMax = count_to_bound(base_size, item_size, origin, hStep, true);

            std::vector<VImage> images{base};
            std::vector<int> modes;
            std::vector<int> xs, ys;

            for (int h = hMin; h <= hMax; h++) {
                int vMin = -count_to_bound(base_size, item_size, origin + hStep * (float) h, -vStep, false);
                int vMax = count_to_bound(base_size, item_size, origin + hStep * (float) h, vStep, false);
                for (int v = vMin; v <= vMax; v++) {
                    images.push_back(watermark);
                    auto pos = origin + vStep * (float) v + hStep * (float) h;
                    xs.push_back((int) pos.x);
                    ys.push_back((int) pos.y);
                    modes.push_back((int) mode);
                }
            }

            state->set_image(arguments.get_string("<slot_out>"), VImage::composite(
                    images,
                    modes,
                    VImage::option()
                            ->set("x", xs)
                            ->set("y", ys)
            ));
        }
};

const Operation cmd_add_alpha{
        "add_alpha",
        R"(
Add an alpha channel to an image, if it doesn't already have one.

Usage:
  @add_alpha <slot_in> <slot_out> [--alpha=<n>]

Options:
  --alpha=<n>  The alpha value to use (0-255) [default: 255]
)",
        [](MachineState *state, const Arguments &arguments) {
            // <slot in = 0> <slot out = 1> <alpha = 2>
            auto input_image = state->get_image(arguments.get_string("<slot_in>"));
            if (input_image.bands() == 4)
                state->set_image(arguments.get_string("<slot_out>"), input_image.copy());
            else
                state->set_image(arguments.get_string("<slot_out>"), input_image
                        .bandjoin_const({arguments.get_double("--alpha")}));
        }

};

const Operation cmd_multiply_color{
        "multiply_color",
        R"(
Multiply the RGBA channels of an image by a constant factor

Usage:
  @multiply_color <slot_in> <slot_out> [--red=<r>] [--green=<g>] [--blue=<b>] [--alpha=<a>]

Options
  -r <r> --red=<r>    Multiply the red channel by this value   (0-255) [default: 255]
  -g <g> --green=<g>  Multiply the green channel by this value (0-255) [default: 255]
  -b <b> --blue=<b>   Multiply the blue channel by this value  (0-255) [default: 255]
  -a <a> --alpha=<a>  Multiply the alpha channel by this value (0-255) [default: 255]
)",
        [](MachineState *state, const Arguments &arguments) {
            auto input_image = state->get_image(arguments.get_string("<slot_in>"));
            state->set_image(arguments.get_string("<slot_out>"), input_image * trim_bands(input_image.bands(), {
                    arguments.get_double("--red"),
                    arguments.get_double("--green"),
                    arguments.get_double("--blue"),
                    arguments.get_double("--alpha")
            }));
        }

};

const Operation cmd_scale{
        "scale",
        R"(
Scale an image

Usage:
  @scale <slot_in> <slot_out> [--horizontal=<f>] [--vertical=<f>]

Options:
  -h <f> --horizontal=<f>  The horizontal scale factor [default: 1]
  -v <f> --vertical=<f>    The vertical scale factor [default: 1]
)",
        [](MachineState *state, const Arguments &arguments) {
            state->set_image(
                    arguments.get_string("<slot_out>"),
                    state->get_image(arguments.get_string("<slot_in>"))
                            .resize(
                                    arguments.get_double("--horizontal"),
                                    VImage::option()
                                            ->set("vscale", arguments.get_double("--vertical"))
                            )
            );
        }
};

const Operation cmd_affine{
        "affine",
        R"(
Apply an affine transform to an image

Usage:
  @affine <slot_in> <slot_out> <m00> <m01> <tx> <m10> <m11> <ty>

The matrix is in the form
  ⎡m00 m01 tx⎤
  ⎢m10 m10 ty⎥
  ⎣0   0   1 ⎦
)",
        [](MachineState *state, const Arguments &arguments) {
            auto input_image = state->get_image(arguments.get_string("<slot_in>"));
            state->set_image(arguments.get_string("<slot_out>"), input_image.affine(
                    {
                            arguments.get_double("<m00>"),
                            arguments.get_double("<m01>"),
                            arguments.get_double("<m10>"),
                            arguments.get_double("<m11>"),
                    },
                    VImage::option()
                            ->set("odx", arguments.get_double("<tx>"))
                            ->set("ody", arguments.get_double("<ty>"))
            ));
        }

};

const Operation cmd_fit{
        "fit",
        R"(
Scale an image to fit inside a region

Usage:
  @fit <slot_in> <slot_out> [--width=<n>] [--height=<n>]

Options:
  -w <n> --width=<n>   The width to fit the image within
  -h <n> --height=<n>  The height to fit the image within
)",
        [](MachineState *state, const Arguments &arguments) {
            auto input = state->get_image(arguments.get_string("<slot_in>"));
            auto scale = -1.0;
            if (arguments.has("--width")) {
                scale = arguments.get_double("--width") / input.width();
            }
            if (arguments.has("--height")) {
                double vscale = arguments.get_double("--height") / input.height();
                if (scale < 0 || vscale < scale)
                    scale = vscale;
            }
            if (scale < 0) {
                state->set_image(arguments.get_string("<slot_out>"), input.copy());
            } else {
                state->set_image(arguments.get_string("<slot_out>"), input.resize(scale, VImage::option()));
            }
        }

};

const Operation cmd_trim_alpha{
        "trim_alpha",
        R"(
Crop transparent edges from an image

Usage:
  @trim_alpha <slot_in> <slot_out> [--threshold=<n>] [--margin=<n>] [--center]

Options:
  -t <n> --threshold=<n>  The alpha threshold to consider "opaque" (0-255) [default: 1]
  -m <n> --margin=<n>     The number of margin pixels to give around the opaque region. This will not add
                          extra margin if the opaque region extends to the edge of the image. [default: 0]
  -c --center             Maintain the image center, cropping uniformly on opposing edges
)",
        [](MachineState *state, const Arguments &arguments) {
            auto input = state->get_image(arguments.get_string("<slot_in>"));

            auto alpha = input.extract_band(3);
            int left, top, width, height;
            left = alpha.find_trim(
                    &top, &width, &height,
                    VImage::option()
                            ->set("threshold", arguments.get_double("--threshold"))
                            ->set("background", std::vector<double>{0})
            );
            int right = input.width() - left - width;
            int bottom = input.height() - top - height;

            int margin = arguments.get_int("--margin");
            left -= margin;
            top -= margin;
            right -= margin;
            bottom -= margin;

            if (arguments.get_bool("--center")) {
                left = right = std::min(left, right);
                top = bottom = std::min(top, bottom);
            }

            if (left < 0) left = 0;
            if (top < 0) top = 0;
            if (right < 0) right = 0;
            if (bottom < 0) bottom = 0;

            width = input.width() - left - right;
            height = input.height() - top - bottom;

            state->set_image(arguments.get_string("<slot_out>"), input.extract_area(left, top, width, height));
        }
};

const Operation cmd_create_image{
        "create_image",
        R"(
Create an image with a flat background color.

Usage:
  @create_image <slot_out> <width> <height> <r> <g> <b> [--alpha=<a>]

Options:
  -a <a> --alpha=<a>  The optional alpha channel to use for the background (0-255)
)",
        [](MachineState *state, const Arguments &arguments) {
            std::vector<double> pixel{
                    arguments.get_double("<r>"),
                    arguments.get_double("<g>"),
                    arguments.get_double("<b>")
            };
            if(arguments.has("--alpha")) {
                pixel.push_back(arguments.get_double("--alpha"));
            }
            state->set_image(arguments.get_string("<slot_out>"),
                             VImage::black(arguments.get_int("<width>"), arguments.get_int("<height>"))
                                     .new_from_image(pixel)
                                     .copy(VImage::option()->set("interpretation", VIPS_INTERPRETATION_sRGB))
            );
        }
};

const Operation cmd_flatten{
        "flatten",
        R"(
Flatten the transparency in an image

Usage:
  @flatten <slot_in> <slot_out> [--red=<r>] [--green=<g>] [--blue=<b>]

Options:
  -r <r> --red=<r>    The red channel to use as the background   (0-255) [default: 0]
  -g <g> --green=<g>  The green channel to use as the background (0-255) [default: 0]
  -b <b> --blue=<b>   The blue channel to use as the background  (0-255) [default: 0]
)",
        [](MachineState *state, const Arguments &arguments) {
            state->set_image(arguments.get_string("<slot_out>"),
                             state->get_image(arguments.get_string("<slot_in>")).flatten(
                                     VImage::option()
                                             ->set("background", std::vector<double>{
                                                     arguments.get_double("--red"),
                                                     arguments.get_double("--green"),
                                                     arguments.get_double("--blue")
                                             })
                             ));
        }
};

const Operation cmd_set_metadata{
        "set_metadata",
        R"(
Set metadata on the image

Usage:
  @set_metadata <slot> <name> <value> <type>

Options:
  <type>  The metadata type
          (see 'vips-tool help enums')
)",
        [](MachineState *state, const Arguments &arguments) {
            auto image = state->get_image(arguments.get_string("<slot>"));
            auto type = arguments.get_string("<type>");
            if (type == "int") {
                image.set(arguments.get_string("<name>").c_str(), arguments.get_int("<value>"));
            } else if (type == "double") {
                image.set(arguments.get_string("<name>").c_str(), arguments.get_double("<value>"));
            } else if (type == "string") {
                image.set(arguments.get_string("<name>").c_str(), arguments.get_string("<value>").c_str());
            } else {
                throw std::invalid_argument(fmt::format("Unrecognized metadata type '{}'", type));
            }
        }
};

const Operation cmd_write{
        "write",
        R"(
Write an image to a file or stream

Usage:
  @write <slot> <file> [--stream-format=<fmt>]

Options:
  -f <fmt> --stream-format=<fmt>  Specify the stream format (e.g. '.jpeg', '.png', '.webp').
                                  (Required for stdout and FIFO streams.)
)",
        [](MachineState *state, const Arguments &arguments) {
            int fd = state->parse_fd(arguments.get_string("<file>"), false);
            if (fd != -1) {
                if (!arguments.has("--stream-format")) {
                    throw std::runtime_error("Writing to a stream requires --stream-format");
                }

                state->get_image(arguments.get_string("<slot>")).write_to_target(
                        arguments.get_string("--stream-format").c_str(),
                        VTarget::new_to_descriptor(fd)
                );
            } else {
                state->get_image(arguments.get_string("<slot>")).write_to_file(
                        arguments.get_string("<file>").c_str()
                );
            }
        }
};

const Operation cmd_consume{
        "consume",
        R"(
Consumes an image, resolving and then discarding all the pixels. Useful mostly for testing.

Usage:
  @consume <slot>
)",
        [](MachineState *state, const Arguments &arguments) {
            size_t size;
            void *memory = state->get_image(arguments.get_string("<slot>")).write_to_memory(&size);
            g_free(memory);
        }

};

const Operation cmd_free{
        "free",
        R"(
Free the image in a slot

Usage:
  @free <slot>
)",
        [](MachineState *state, const Arguments &arguments) {
            state->free_image(arguments.get_string("<slot>"));
        }

};

const Operation cmd_copy{
        "copy",
        R"(
Copy the image from one slot into another slot

Usage:
  @copy <source_slot> <dest_slot>
)",
        [](MachineState *state, const Arguments &arguments) {
            state->set_image(
                    arguments.get_string("<dest_slot>"),
                    state->get_image(arguments.get_string("<source_slot>"))
            );
        }

};

const Operation cmd_resolve{
        "resolve",
        R"(
Resolve a slot into memory. This is useful for accessing a sequential-only image (e.g. jpeg read) multiple times, which
otherwise would crash.

Usage:
  @resolve <slot>
)",
        [](MachineState *state, const Arguments &arguments) {
            vips_image_wio_input(state->get_image(arguments.get_string("<slot>")).get_image());
        }
};

const Operation cmd_set_var{
        "set_var",
        R"(
Set a variable to a value

Usage:
  @set_var <var> <value>
)",
        [](MachineState *state, const Arguments &arguments) {
            state->set_variable(arguments.get_string("<var>"), arguments.get_double("<value>"));
        }
};

const Operation cmd_print{
        "print",
        R"(
Print a value

Usage:
  @print <label> <value>

The output will be sent to stderr in the form '@<name>: <value>\n'.
stderr is used because stdout may be used for image streaming.
)",
        [](MachineState *state, const Arguments &arguments) {
            fmt::print(stderr, "@{}: {}\n", arguments.get_string("<label>"), arguments.get_double("<value>"));
        }
};

const Operation cmd_phash{
        "phash",
        R"(
Compute the perceptual hash of an image

Usage:
  @phash <slot> --reduce=<n> --sample=<n> [--label=<label>]

Options:
  -r <n> --reduce=<n>         The size to reduce the image to before computing the DCT
  -s <n> --sample=<n>         The size to sample from the DCT image. The output will be n^2 bits.
  -l <label> --label=<label>  The label to use. Defaults to the slot name.

The output will be sent to stderr in the form:
  '@<label>: <version>:<reduce size>:<sample size>:<bit string>'
stderr is used because stdout may be used for image streaming.
)",
        [](MachineState *state, const Arguments &arguments) {
            auto slot = arguments.get_string("<slot>");

            auto reduce = arguments.get_int("--reduce");
            auto sample = arguments.get_int("--sample");
            auto hash = pHash(
                    state->get_image(slot),
                    reduce,
                    sample
            );

            std::string bitString(hash.bits.size(), '0');
            for (auto i = 0; i < hash.bits.size(); i++) {
                bitString[i] = hash.bits[i] ? '1' : '0';
            }
            fmt::print(
                    stderr, "@{}: {}:{}:{}:{}\n",
                    arguments.get_string("--label", slot),
                    hash.version, reduce, sample,
                    bitString
            );
        }
};

#define OP(op) {op.name, &op}
const std::map<std::string, const Operation *> operations = {
        OP(cmd_load),
        OP(cmd_thumbnail),
        OP(cmd_write),
        OP(cmd_phash),
        OP(cmd_profile),
        OP(cmd_unsharp),
        OP(cmd_autorotate),
        OP(cmd_create_image),
        OP(cmd_flatten),
        OP(cmd_add_alpha),
        OP(cmd_scale),
        OP(cmd_affine),
        OP(cmd_fit),
        OP(cmd_multiply_color),
        OP(cmd_grid),
        OP(cmd_trim_alpha),
        OP(cmd_composite),
        OP(cmd_set_metadata),
        OP(cmd_consume),
        OP(cmd_free),
        OP(cmd_copy),
        OP(cmd_resolve),
        OP(cmd_set_var),
        OP(cmd_print),
        OP(cmd_open_stream),
};
#undef OP

const Operation *get_operation(const std::string &name) {
    auto operation = operations.find(name);
    if (operation == operations.end()) {
        return nullptr;
    } else {
        return operation->second;
    }
}

struct ImageFunction : public double_function {
    typedef double (*callback_t)(const VImage &);

    MachineState *state;
    callback_t callback;

    ImageFunction(MachineState *state, callback_t callback) : state(state), callback(callback), double_function("S") {}

    inline double operator()(double_function::parameter_list_t parameters) override {
        double_string_t name_view(parameters[0]);
        std::string name(name_view.begin(), name_view.size());
        auto image = state->get_image(name);
        return callback(image);
    }
};

void initialize_functions(MachineState *state) {
    state->add_function("vips_width", new ImageFunction(state, [](auto image) { return (double) image.width(); }));
    state->add_function("vips_height", new ImageFunction(state, [](auto image) { return (double) image.height(); }));
    state->add_function("vips_bands", new ImageFunction(state, [](auto image) { return (double) image.bands(); }));
}


#pragma clang diagnostic pop