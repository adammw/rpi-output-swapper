Video Output Swapper for Raspberry Pi
=====================================

An easy tool to swap between SDTV and HDMI outputs

Usage
-----

<pre>
Usage: ./video_swap.bin [OPTION]...
--help 		            : display this usage information
--version 	            : print version information
--status 	            : print current resolution / and output device
--no-fbset 	            : don't call fbset to reset the framebuffer
--sdtv [mode] [aspect]  : switch output to sdtv
--hdmi [group] [mode]   : switch output to hdmi with the specified mode and type,
                          otherwise defaults to the monitor's preferred mode
</pre>

Instructions
------------

To run the tool, simply clone the git repository and run make with the following command:

<pre>
git clone https://github.com/adammw/rpi-output-swapper.git
make
</pre>

You can then run the tool directly from the folder or add it to your bin directory or PATH.
