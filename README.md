# Foxotron

[![GitHub Workflow Status](https://img.shields.io/github/workflow/status/Gargaj/Foxotron/build-on-push?logo=github)](https://github.com/Gargaj/Foxotron/actions)

ASSIMP based general purpose model viewer ("turntable") created for the Revision 2021 3D Graphics Competition

## Usage
Check [the wiki](https://github.com/Gargaj/Foxotron/wiki) for information on how to use it.

### Keyboard shortcuts
* F11: Toggle menu
* F: Refocus camera on mesh
* W: Toggle wireframe / edged faces
* C: Toggle auto camera
* PgUp / PgDn: Cycle through shaders
* Alt-F4: Enter hyberwormtunnel

### Mouse operations
* Left click & drag: Rotate camera
* Right click & drag: Rotate light / skybox
* Middle click & drag: Pan camera

## Requirements
OpenGL 4.1 is required.

## Building
You're gonna need [CMAKE](https://cmake.org/)

## Credits

### Libraries and other included software
- Open Asset Import Library by the ASSIMP dev team (https://www.assimp.org)
- Dear ImGui by Omar Cornut (http://www.dearimgui.com)
- ImGui Addons by gallickgunner (https://github.com/gallickgunner/ImGui-Addons)
- OpenGL Extension Wrangler Library by Nigel Stewart (http://glew.sourceforge.net)
- STB Image library by Sean Barrett (https://nothings.org)
- GLFW by whoever made GLFW (https://www.glfw.org/faq.html)

These software are available under their respective licenses.

The remainder of this project code was (mostly) written by Gargaj / Conspiracy and is public domain; PBR lighting shaders by cce / Peisik.
Large portions of the code were cannibalized from [Bonzomatic](https://github.com/Gargaj/Bonzomatic).

### Textures and cube maps

All HDR textures are courtesy of the [HDRLabs sIBL archive](http://www.hdrlabs.com/sibl/archive.html).
