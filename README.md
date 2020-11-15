# GL Points

### Requirements

`libGL`, `libGLEW`

`libglfw` is included, since I use raw mouse input, which is only available in very recent versions.

### Build

`make`

### Run

`./run.sh`

Running the binary directly won't work, since the included `libglfw` version is not in the `LD_LIBRARY_PATH`
