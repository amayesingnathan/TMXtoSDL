#pragma once

#include "pch.h"

#include "Layer.h"

struct TilesetData
{
	SDL_Texture* tilesetTex;
	int tileWidth;
	int tileHeight;
	int tilesetWidth;
};

using Collider = SDL_Rect;
using ColliderList = std::vector<Collider>;

class TiledIO
{
public:
	static void SetRenderer(SDL_Renderer* renderer) { mCurrentRenderer = renderer; }
	static void OpenLevel(const std::filesystem::path& lvl, std::vector<Layer>& layerList, std::unordered_map<int, TilesetData>& tilesets, std::unordered_map<int, ColliderList>& tilesetColliders, SDL_Renderer* renderer = nullptr);

private:
	static void GetLayers(rapidxml::xml_node<>* mapNode, std::vector<Layer>& layerList);
	static void GetTilesets(rapidxml::xml_node<>* mapNode, const std::filesystem::path& lvlPath, std::unordered_map<int, TilesetData>& setData, std::unordered_map<int, ColliderList>& tilesetColliders);
	static void GetTileData(const std::filesystem::path& tileset, int firstID, std::unordered_map<int, TilesetData>& setData, std::unordered_map<int, ColliderList>& tilesetColliders);
	static void GetTileData(rapidxml::xml_node<>* tilesetNode, int firstID, const std::filesystem::path& pngPath, std::unordered_map<int, TilesetData>& setData, std::unordered_map<int, ColliderList>& tilesetColliders);
		
	static rapidxml::xml_node<>* GetChild(rapidxml::xml_node<>* inputNode, std::string sNodeFilter);
	static ColliderList GetColliders(rapidxml::xml_node<>* inputNode);

private:
	static SDL_Renderer* mCurrentRenderer;
};
