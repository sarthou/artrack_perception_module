# ArTrack Perception Module

[![Dependency Status][Overworld-Dependency-Image]][Overworld-Dependency-Url]
[![Dependency Status][ArTrack-Dependency-Image]][ArTrack-Dependency-Url]

This package is an Overworld object perception module using the Ar_track_alvar system.

We advise you to install all Overworld external perception modules in a same folder (e.g. overworld_modules) in your ROS workspace.

## Installation

Here we assume Overworld to be installed.

 - ROS Melodic:

```bash
cd ~/catkin_ws/src
git clone https://github.com/sarthou/ar_track_alvar.git
cd overworld_modules
git clone https://github.com/sarthou/artrack_perception_module.git
cd ../..
catkin_make
```

 - ROS Noetic:

```bash
cd ~/catkin_ws/src
git clone -b noetic-devel https://github.com/sarthou/ar_track_alvar.git
cd overworld_modules
git clone https://github.com/sarthou/artrack_perception_module.git
cd ../..
catkin_make
```

[Overworld-Dependency-Image]: https://img.shields.io/badge/dependencies-overworld-yellowgreen
[Overworld-Dependency-Url]: https://github.com/sarthou/overworld

[ArTrack-Dependency-Image]: https://img.shields.io/badge/dependencies-ar_track_alvar-yellowgreen
[ArTrack-Dependency-Url]: https://github.com/sarthou/ar_track_alvar