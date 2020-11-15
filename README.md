# GL Points

### Requirements

`libGL` & `libGLEW`.

`libglfw` is included with the code, since I use raw mouse input, which is only available in very recent versions.

### Build

`make`

### Run

`./run.sh`

Running the binary directly won't work (Unless you have a new enough version of `libglfw` installed), since the included `libglfw` is not in the `LD_LIBRARY_PATH`.
