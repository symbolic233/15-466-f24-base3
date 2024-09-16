#!/bin/bash
"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python export-scene.py -- $1.blend:Main $1.scene
"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" --background --python export-meshes.py -- $1.blend:Main $1.pnct