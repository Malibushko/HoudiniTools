# HoudiniTools

A collection of custom nodes for Houdini — built to extend Houdini’s toolset with new functionality.

## 🧱 Architecture & Build

- Source code lives under `src/`  
- Uses CMake for build configuration.  
- Targeted for Houdini HDK environment (matching Houdini version required).  
- License: GPL-3.0. (See `LICENSE` file).  

## 🔧 Nodes List (currently implemented)

| # | Node Name | Description | Status | 
|---|-----------|-------------|--------| 
| 1 | Convex2D  | Constructs a convex hull for a set of 2D points | Done 
| 2 | Intersection analysys | Finds intersection for a bunch of segments (brute force or Bentley-Ottoman) | Done (can be improved)
