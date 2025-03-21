# threebody-simulation  

## Learning outcomes
- Practice development in C++
- Learn OpenGL
- Apply my mathematical background to learn some basic physics

## Dependencies

### C++
`sudo apt install libstdc++-12-dev`

### OpenGL
`sudo apt install cmake pkg-config mesa-utils libglu1-mesa-dev freeglut3-dev mesa-common-dev libglew-dev libglfw3-dev libglm-dev libao-dev libmpg123-dev`

### Wayland and X11
`sudo apt install libwayland-dev libxkbcommon-dev xorg-dev`

## OpenGL setup

### GLFW library
cd /usr/local/lib/
sudo git clone https://github.com/glfw/glfw.git
cd glfw
sudo cmake .
sudo make
sudo make install

### GLAD library
Access https://glad.dav1d.de/
Configure like the image below
![glad settings](glad.png)
Generate the zip file
Copy the directories inside of the directory /include (/glad and /KHR) into the system's /include directory 
`cp -R include/* /usr/include/`
Move the file glad.c inside the src/ directory into the current working directory
`mv src/glad.c /working/directory`

### Compile
`g++ sim.cpp glad.c -ldl -lglfw`

## Physics concepts
### Gravity

### Velocity

### Acceleration

### Relativity

## Out of scope
Actual analysis of the trajectories and or determining if a system with three suns has periodicity
