# TMXtoSDL
This is a library for extracting tile and collision data from a Tiled .tmx map file. If you already have SDL and RapidXML set up in your project then it can be used as a header only library. Include directories may need to be altered in this case.

A .tmx file is a Tiled map file encoded in XML. This library uses RapidXML to parse the file, and converts the data to SDL2 usable structures.

This library requires:
* An SDL_Renderer pointer for generating the tile SDL_Textures - this can be set by using `IO::SetRenderer(SDL_Renderer* renderer)`, or can be passed in as the last argument of `IO::OpenLevel(...)`.
* The .tmx file to be stored in a directory with the same name as it, which also contain all addition tileset (.tsx) files and textures (.png) referenced in the map file.

The `IO::OpenLevel` function takes in the following parameters:
* lvlPath - A `std::filesystem::path` to the level directory.
* layerList - The output vector of Layers, each containing the tile ID at each position of the tilemap.
* tilesetData - The output vector of Tilesets. It contains the tileID of the first tile of the set, the size of each tile, and the number of tiles per row.
* tilesetColliders - The output map of tile IDs to the list of SDL_Rects that make up its colliders. The position of the colliders is relative to the tile.
* renderer - Null by default, but required if SetRenderer has not already been used. The function will produce no output if it no renderer is provided.

Use `TilesetData* FindTilesetData(int tileID, std::vector<TilesetData>& tilesets)` function to extract a pointer to the tileset which the given tileID belongs to.
