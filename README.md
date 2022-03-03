# Arc04 - 3D game engine
Has skeletal animation, collision detection (no BVH yet), raycasting, has a basic real time editor and a tool for baking assets to binary format.
Here's a makeshift video of it working:

https://www.youtube.com/watch?v=ndrV567eVd4

## Build
Run setup.bat (just once) and fbuild.exe.
Build steps include auto-generated struct 'instrospection' code!
## Compile data
Run bin/bakery.exe to process the game data. Bakery will detect changes on data files and rebuild accordingly.
## Run
Run bin/game.exe
