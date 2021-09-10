# c-vips-pipeline

## About The Project

This is a simple application to interact with the VIPS API in order to perform a simple scale
of an input image and subsequently perform sharpening on the scaled image before output. The
motivation for creating this simple application rather than simply using the `vipsthumbnail`
or `vips` is: (1) `vipsthumbnail` does not provide control over the sharpening used to create
its output thumbnails, and (2) the `vips` CLI requires creating intermediate files on disk in
order to make multiple calls to the VIPS API, which would negatively affect performance
dramatically.

## Commands

For optional values, an underscore (`_`) represents no value.

```
@thumbnail <file in> <slot out> <width> <height?> <no-rotate> <intent?>
@profile <slot in> <slot out> <profile>
@unsharp <slot in> <slot out> <sigma> <strength>

@load <file in> <slot out>
@composite <base slot> <overlay slot> <slot out> <overlay x> <overlay y> <blend mode>
@embed <slot in> <slot out> <x> <y> <width> <height> <extend> <bg red> <bg green> <bg blue>
 
@flatten <slot in> <slot out> <background red> <background green> <background blue>
@add_alpha <slot in> <slot out>
@scale <slot in> <slot out> <hscale> <vscale>
@fit <slot in> <slot out> <width?> <height?>
@trim_alpha <slot in> <slot out> <threshold> <margin>
@multiply_color <slot in> <slot out> <r> <g> <b> <a>

@mime_jpeg <slot>
@mime_webp <slot>
@write <slot in> <file out>
@consume <slot>
@free <slot>
```
