# HPC Disease Simulation

This is the template project for the HPC-05 Programming Lab project "Disease Simulation".

## Compilation 

To compile the project, run 

```
mkdir build && cd build
cmake ..
make
```

## Run 

There are different targets and binaries available to execute. 

### The Simulation
After compilation, you can run the simulation using:

```
./hpc_disease_simulation
```


### The Tests 
You can run all tests via: 

```
make test
```

or 

```
./all_tests
```

### Test Coverage 

To calculate the test coverage (target 70% for own code) you need to re-run `cmake`: 

```
cmake .. -DENABLE_COVERAGE=ON
make coverage
```

After this, you can open the web report in `./coverage/index.html`
