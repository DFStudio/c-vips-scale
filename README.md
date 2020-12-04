# c-vips-pipeline

## About The Project

This is a simple application to interact with the VIPS API in order to perform a simple scale
of an input image and subsequently perform sharpening on the scaled image before output. The
motivation for creating this simple application rather than simply using the `vipsthumbnail`
or `vips` is: (1) `vipsthumbnail` does not provide control over the sharpening used to create
its output thumbnails, and (2) the `vips` CLI requires creating intermediate files on disk in
order to make multiple calls to the VIPS API, which would negatively affect performance
dramatically.