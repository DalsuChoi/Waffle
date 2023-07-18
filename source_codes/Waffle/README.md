# Waffle

### Prerequisites
- C++14
- NVIDIA GPU
- PyTorch with the PyTorch C++ frontend (libtorch)
- Intel® oneAPI Threading Building Blocks
- Restbed

### Experimental Environments
- Ubuntu 20.04.6 LTS (5.15.0-72-generic)
- Intel(R) Core(TM) i9-9900K CPU @ 3.60GHz
- 64 GB main memory
- 500 GB NVMe SSD
- NVIDIA GeForce RTX 3080
- PyTorch 1.13.1

### How to Build
<pre>
<code>
mkdir Release
cd Release
cmake .. -D CMAKE_PREFIX_PATH="PATH_TO_LIBTORCH;PATH_TO_TBB"
make
</code>
</pre>

### How to Execute Waffle
- Iuput parameters
  - The absolute path to a query file
  - The maximum number of objects
- For example, "./Waffle /Waffle_dataset/LA 2000000"

### Query
Waffle reads queries from a single file, which consists of the following elements.
- The first query should be "SETSPACE minimum_longitude minimum_latitude maximum_longitude maximum_latitude" to define a geographical space in Section 2 [1].
  - For example, "SETSPACE -118.766 33.844 -117.793 34.324"
- Insertion query
  - INSERT object_ID longitude latitude
  - For example, "INSERT 1 -117.891 34.049"
  - If the target object already exists in a Waffle index, Waffle handles the query as a movement query, as explained in Section 5 [1].
- Deletion query
  - DELETE object_ID
  - For example, "DELETE 1"
- Range query
  - RANGE start_longitude start_latitude end_longitude end_latitude
  - For example, "RANGE -118.544 34.188 -118.538 34.193"
- k-NN query
  - KNN start_longitude start_latitude end_longitude end_latitude k
  - For example, "KNN -118.544 34.188 -118.538 34.193 10"
- The final query should be "TERMINATE".
- object_ID should be between 0 and (the maximum number of objects - 1).
  
### System Parameters
The predefined system parameters, which are not automatically tuned by Waffle, are set in 'include/Parameters.h'.

### Softwares Used without Modification of Source Codes
- PyTorch [2]
- Intel® oneAPI Threading Building Blocks [3]
- Restbed [4]
- JSON for Modern C++ [5]

### References
[1] Dalsu Choi, Hyunsik Yoon, Hyubjin Lee, and Yon Dohn Chung. Waffle: In-memory Grid Index for Moving Objects with Reinforcement Learning-based Configuration Tuning System. PVLDB, 15(11): 2375-2388, 2022.<br>
[2] https://pytorch.org/<br>
[3] https://github.com/oneapi-src/oneTBB<br>
[4] https://github.com/Corvusoft/restbed<br>
[5] https://github.com/nlohmann/json<br>
