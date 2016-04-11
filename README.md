# Cache Friendly Proximity Query

Copyright 2016(c) Wael El Oraiby, All rights reserved. 

![random](https://github.com/eloraiby/ProximityQuery/raw/master/screenshot.png)

### Motivation
If you are working in CAD, 3D modeling, animation or simulation you will probably need a library that performs proximity queries as fast as they can come. This project aims to find the closest point on a triangular mesh, which is useful for operations like sculpting or lighting. 

The traditional algorithms and their implementations rarely take memory transfer overhead and multi-core processors in consideration. Most of them were developed back in the days where CPUs were on par with external memory when it comes to speed.
There is however a way to extract the maximum performance out of the CPU by taking advantage of the cache memory using Data Oriented Algorithms.

### Technique
Nowadays processors have many cores and are fast. In fact, their speed is about 100x times greater than the external memory they feed on.
To remedy this problem, they use multiple levels of cache memory. These are the number taken from `Core i7 Xeon 5500 Series Data Source Latency (approximate)[Pg. 22]`
and the related stackoverflow question: http://stackoverflow.com/questions/4087280/approximate-cost-to-access-various-caches-and-main-memory

- `local  L1 CACHE hit, ~4 cycles(2.1 - 1.2 ns)`
- `local  L2 CACHE hit, ~10 cycles(5.3 - 3.0 ns)`
- `local  L3 CACHE hit, line unshared               ~40 cycles(21.4 - 12.0 ns)`
- `local  L3 CACHE hit, shared line in another core ~65 cycles(34.8 - 19.5 ns)`
- `local  L3 CACHE hit, modified in another core    ~75 cycles(40.2 - 22.5 ns)`

- `local  DRAM                                                   ~60 ns`
- `remote DRAM                                                  ~100 ns`


To this aim, this program was developed. The idea is quite simple, but quite effective:

1 - Construct a Bounded Volume Hierarchy **BVH**

2 - Separate the nodes from the leaves (Data Orientation) :
  The distinction is that leaves are a batch of triangles __instead__ of a single or few triangles.
  * The node data is usually small and fits in L1-cache memory.
  * If the radius is small comparing to the mesh size, The nearest leaves fit in L2/L3-cache. This has the __nice consequence__ of letting the CPU/MMU make consecutive and relatively close queries faster since the data is warm in the cache.
  * Data orientation has the benefit of making this algorithm thread safe since all the methods/functions are const and re-entrant (functional programming technique for multi-threading: once the query-mesh is constructed the queries are all ReadOnly operations).

### The Program/UI
The UI should allow you to test and experiment with the proximity query:
- Spherical coordinates are used to control the location of the proximity query center:
  - `sc Radius`  : is the spherical coordinate radius
  - `sc Azimuth` : the azimuth
  - `sc Polar`   : the polar coordinate
  
- Rotation is axis/angle based and is controllable with the sliders.

- The maximum triangle count per leaf (a hint) is controllable and updates the mesh **on the fly**

### Code
The meat of the algorithm are in TriMesh.hpp and TriMesh.cpp. The other files are helpers for visualization or 3rd party libraries.
  
The code is made to be as data oriented as possible except for the construction phase where it will allocate memory on the fly.
  
The heuristics for building the **BVH** is simple and explained in the code
  
Most of the fun parts are documented in the code.
  
