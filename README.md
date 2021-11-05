# Laser Is Dangerous
'Laser Is Dangerous' is a fast paced 2D laser-dodging & shooting game where you have to fight with waves of enemies. Move against gravity while avoid everything - including the enemies, lasers, mountains, and walls.

Inspired by Handmade Hero show by Casey Muratori, the engine is build without using any kind of third party library.

# Why?
Third party libraries are meant to save you from unwanted works, so that you can focus on what you really want to do. 
However, a lot of the times, I found out that by using those libraries, people(including myself) tend to do more guessing than actually knowing how the code works.
This game is a result of myself working & studying while avoiding any kind of libraries, so that I can fully understand how the actual game engine is built from scratch.

# How to build
1. Open Command Prompt For Visual Studio(x64 Native Tool or x64_x86 Cross Tool)
2. Nagivate to the 'laser_is_dangerous/code'
3. Execute build.bat

Your build files will be located at the build folder, outside the laser_is_dangerous folder. Enjoy!

# Features
- Custom Windows porting, without using libraries such as GLFW
- Custom Asset Builder & Loader, which loades BMP & WAV files and pack them into a single asset file
- SIMD based software renderer and OpenGL
- Custom 2D physics engine
- Runtime code change
