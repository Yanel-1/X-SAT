## X-SAT

An Efficient Circuit-Based SAT Solver

### Datasets

The circuits mentioned in the paper are stored in the `data` directory, divided into arithmetic circuits and non-arithmetic circuits.

### Usage

#### Compile

```bash
mkdir build
cd build
cmake ..
make -j
```

The executable file `csat` can be found in the current directory.

#### Run

Using structral elimination and XVSIDS you should run:

```
./csat -i XXXXXXXXXX.aiger
```

If you want to test other strategies, please refer to the help.

```
usage: ./csat --instance=string [options] ... 
options:
  -i, --instance            .aig/.aag format instance (string)
  -p, --pre_out             enable preprocess and write cnf to dist (string [=])
      --reduce_limit_inc    reduce_limit_inc (int [=1024])
      --reduce_per          reduce_per (int [=50])
  -b, --branchMode          0 for vmtf, 1 for VSIDS, 2 for JFRONTIER (int [=1])
  -l, --max_lut_input       LUT input limit (int [=7])
  -e, --enable_elim         enable elimination (int [=1])
  -x, --enable_xvsids       enable xor bump for VSIDS (int [=1])
  -?, --help                print this message
```