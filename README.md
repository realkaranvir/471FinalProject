# CPE 471 Graphics Final Project - 3D Obstacle Course

## Project Overview
This is a 3D obstacle course game built using OpenGL, featuring character movement, obstacle navigation, and dynamic camera controls. Navigate through a desert landscape filled with cacti, spherical obstacles, spinning axes, and challenging platforming sections to reach the end goal.

## Controls

### Character Movement
- **Left Arrow Key** - Move character left
- **Right Arrow Key** - Move character right  
- **Spacebar** - Jump

### Camera Controls
- **T** - Toggle between third-person and first-person camera modes
- **W/A/S/D** - Free camera movement (first-person mode)
  - W: Move camera forward
  - A: Move camera left
  - S: Move camera backward
  - D: Move camera right
- **F** - Lower camera height
- **Backspace** - Raise camera height
- **Mouse Scroll** - Adjust camera angle (pitch and yaw)

### Special Features
- **G** - Activate cinematic camera spline animation
- **R** - Reset game (character position, camera, and game state)
- **Q/E** - Adjust lighting position
  - Q: Move light source right
  - E: Move light source left
- **M** - Toggle material properties for starting platform
- **Z** - Hold to enable wireframe mode

### Debug/Utility
- **Escape** - Exit the game

## Game Mechanics

### Objective
Navigate your character from the starting platform to the end goal while avoiding obstacles and hazards.

### Physics System
- **Gravity**: Character falls when not supported by ground or platforms
- **Collision Detection**: Character interacts with spherical obstacles, rectangular platforms, and hazardous spinning axes
- **Jump Mechanics**: Single jump system with realistic physics

### Obstacles & Hazards
- **Spinning Axes**: Rotating metallic axes that will end the game if touched during their swing
- **Spikes**: Ground-level hazards (visual elements)
- **Pillars & Platforms**: Structural elements that provide jumping surfaces

### Collision System
- **Platform Collision**: Character can stand on top of platforms and spheres
- **Wall Collision**: Character is pushed away from solid objects when approaching from the side
- **Hazard Detection**: Contact with spinning axes during their active swing results in game over
- **Victory Condition**: Reaching the end position (x ≥ 59.0, y ≥ 5.5) triggers win state

### Visual Features
- **Textured Environment**: Multiple textures including stone, wood, marble, and metal
- **Dynamic Lighting**: Adjustable light source affects all rendered objects
- **Skybox**: 360-degree desert environment background
- **Character Animation**: Simple walking animation with arm and leg movement

## Game States

### Playing
- Normal gameplay with full character control and physics
- Camera follows character in third-person mode by default

### Game Over (Lost)
- Triggered by falling on the spikes or touching spinning axes
- Use 'R' to reset and try again

### Victory (Won)
- Triggered by reaching the end goal
- Use 'R' to reset and play again

### Cinematic Mode
- Activated with 'G' key
- Camera follows predetermined spline path
- Character control disabled during animation
- Automatically returns to normal mode when complete

## Technical Features
- **OpenGL Rendering**: Modern OpenGL with shader programs
- **Texture Mapping**: Multiple texture units for different materials
- **Matrix Transformations**: Hierarchical model transformations
- **Spline Animation**: Bezier curve camera movement

## Tips for Playing
1. Use the third-person camera mode for better spatial awareness
2. Time your jumps carefully around the spinning axes
3. The spherical obstacles can be used as stepping stones
4. Practice the jump timing - you can only jump when on solid ground
5. Use the reset key 'R' frequently to restart if you get stuck
6. Experiment with the free camera mode to explore the scene

## Development Notes
This project demonstrates various computer graphics concepts including 3D transformations, texture mapping, lighting models, collision detection, and interactive camera systems.