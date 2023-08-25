# Termite

## What is Termite?
Termite is a library concerned with the management of memory and threads in an OS-agnostic
manner. Currently, only the thread management portion is (relatively) functional.

## How to build
Termite is available to be built both as a CMake project and a Visual Studio project. 

## Dependencies
Termite depends only on Vulkan, for some memory related features. That's because it is planned to be used alongside another project of mine, [Dragonfly](https://github.com/xmamalou/dragonfly). However, in the future, I will try to make it as such that the Vulkan specific features are optional.