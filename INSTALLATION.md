# WSN Simulation Module - Installation Guide

## Prerequisites

Before installing the WSN module, ensure you have the following:
- **ns-3.46** (ns-3-dev-git version 3.46)
- **C++ compiler** (GCC 5.0+)
- **CMake** 3.10+
- **Python 3.x**

## Installation Steps

### Step 1: Download ns-3-dev-git 3.46

Clone or download the ns-3.46 repository:

```bash
git clone https://gitlab.com/nsnam/ns-3-dev-git.git
cd ns-3-dev-git
git checkout ns-3.46
```

### Step 2: Place WSN Module in src Directory

Copy the entire `wsn` folder into the `src` directory of your ns-3 installation:

```bash
cp -r /path/to/wsn-module src/wsn
```

Or if you're cloning directly:
```bash
cd ns-3-dev-git/src
git clone <wsn-module-repo> wsn
```

### Step 3: Configure ns-3 with WSN Module

Run the configuration command to enable the WSN module:

```bash
./ns3 configure --enable-examples --enable-modules=wsn
```

### Step 4: Build the Project

Compile the ns-3 framework with the WSN module:

```bash
./ns3 build
```

This may take several minutes depending on your system specifications.

### Step 5: Run the WSN Simulation

Execute the WSN simulation:

```bash
./ns3 run wsn-sim
```

## Verification

If the installation is successful, you should see:
- Build completes without errors
- Simulation starts and outputs rotation messages
- Messages showing CH rotation cycles and duration measurements

Example output:
```
======== CH ROTATION #1 START at 20s ========
#CH_ROTATE Cell:0 OldCH:1 NewCH:0
...
#CHA_COMPLETED_ROTATION #1 Duration:3.43475s (LastSendAt:23.4348s, CHsActive:158)
```

## Configuration

The simulation uses configuration files in the `ns-3-dev-git` root directory:
- `input-pecee.ini` - Main configuration file
- `ns3` - ns-3 build system script

To modify the number of nodes or other parameters, edit `input-pecee.ini`:

```ini
SN.numNodes = 100  # Change to 400, 1024, etc.
sim-time-limit = 100s  # Simulation duration
```

## Troubleshooting

### Build Fails
If the build fails, try:
```bash
rm -rf cmake-cache build
./ns3 configure --enable-examples --enable-modules=wsn --disable-precompiled-headers
./ns3 build
```

### Module Not Found
Ensure the `wsn` folder is correctly placed in `src/`:
```bash
ls src/wsn/
# Should show: CMakeLists.txt, model, examples, etc.
```

### Runtime Issues
- Ensure you're in the ns-3-dev-git root directory when running `./ns3 run wsn-sim`
- Check that `input-pecee.ini` exists in the root directory

## Performance Notes

Simulation execution time varies by scenario:
- **100 nodes**: ~5 seconds per rotation, ~5s real time to first rotation
- **400 nodes**: ~3.4 seconds per rotation, ~5s real time to first rotation
- **1024 nodes**: ~4.8 seconds per rotation, ~42s real time to first rotation

## Support

For issues or questions, refer to:
- ns-3 documentation: https://www.nsnam.org/
- WSN module documentation in the `doc/` directory

## License

This WSN module is developed as part of a graduation thesis project. 