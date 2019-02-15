# Memana
An implementation of various memory management algorithms(first/next/best/worst fit).

It only use the C's malloc once to obtain the basic memory, not additionaly allocations needed for the memory of nodes or anything.
Nodes used in the algorithm are directly encoded into the initial memory and manipulated by the C's pointer operations.

-----

It's an assignment in my operating system class. Implementing those algorithms and comparing its performance.

There is a description of the problem, but it's in chinese. Also the code are well-documented in chinese. 

I worte so many comments for my professor's reading, but he didn't spend much time in it. I'm really disappointed. :(

## Usage
The running results are just some plain numbers showing the time each algorithm used in the given problem.

There is a data genertor written in python. Run it and you can get a new data file.
```shell
make all
./first
./next
./best
./worst
```
