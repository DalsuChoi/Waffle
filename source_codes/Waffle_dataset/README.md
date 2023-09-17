# Dataset

### Prerequisites
- C++14
- Eigen

### Experimental Environments
- Ubuntu 20.04.6 LTS (5.15.0-72-generic)
- Intel(R) Core(TM) i9-9900K CPU @ 3.60GHz
- 64 GB main memory
- 500 GB NVMe SSD

### How to Build
<pre>
<code>
cd Waffle_dataset
mkdir Release
cd Release
cmake ..
make
</code>
</pre>

### How to Generate the Queries Used in the Paper
1. Download road network dataset from UCR STAR [1].
- Open the website through the link [1].
- Set the screen to the target geographical space.
- Click 'Download data -> Visible area -> CSV'.
- Or download LA road network dataset from the link: https://1drv.ms/u/s!AnIVNfovuSvYgTVfGZPiNzAuefb9?e=GdfGz4

2. How to Execute
- Iuput parameters
  - The absolute path to the road dataset
  - The absolute path to the output file
  - The number of episodes
- For example, "./Waffle_dataset /data/OSM2015_LA /data/LA 100"

### How to Visualize Objects
1. Set 'output_objects_to_file' to true and 'path_to_objects_file' in 'parameters.h'.
2. Build and execute based on the above steps.
3. For each object file (e.g., Objects_Episode0_Tick1), run 'python visualizeObjects.py path/Objects_Episode0_Tick1'. A png file will be generated at the same directory as that of the object file.

### Softwares Used without Modification of Source Codes
- Eigen

### References
[1] Ahmed Eldawy and Mohamed F. Mokbel. 2019. Roads and streets around the world each represented as individual line segments. https://doi.org/10.6086/N1H99379#mbr=9qwesvg4,9qxq6f81 Retrieved from UCR-STAR https://star.cs.ucr.edu/?OSM2015/road-network.
