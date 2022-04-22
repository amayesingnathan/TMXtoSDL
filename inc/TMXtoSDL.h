#pragma once

#include "pch.h"

#include "Layer.h"

struct TilesetData
{
	int firstID;
	SDL_Texture* tilesetTex;
	int tileWidth;
	int tileHeight;
	int tilesetWidth;

	TilesetData() = default;
	TilesetData(int id, SDL_Texture* tex, int w, int h, int setW)
		: firstID(id), tilesetTex(tex), tileWidth(w), tileHeight(h), tilesetWidth(setW) {}

	bool operator <(const TilesetData& other) const
	{
		return firstID < other.firstID;
	}
};

using Collider = SDL_Rect;
using ColliderList = std::vector<Collider>;

class TiledIO
{
public:
	static void SetRenderer(SDL_Renderer* renderer) { mCurrentRenderer = renderer; }
	static void OpenLevel(const std::filesystem::path& lvlPath, std::vector<Layer>& layerList, std::vector<TilesetData>& tilesetData, std::map<int, ColliderList>& tilesetColliders, SDL_Renderer* renderer = nullptr);

private:
	static void GetLayers(rapidxml::xml_node<>* mapNode, std::vector<Layer>& layerList);
	static void GetTilesets(rapidxml::xml_node<>* mapNode, const std::filesystem::path& lvlPath, std::vector<TilesetData>& setData, std::map<int, ColliderList>& tilesetColliders);
	static void GetTileData(const std::filesystem::path& tileset, int firstID, std::vector<TilesetData>& setData, std::map<int, ColliderList>& tilesetColliders);
	static void GetTileData(rapidxml::xml_node<>* tilesetNode, int firstID, const std::filesystem::path& pngPath, std::vector<TilesetData>& setData, std::map<int, ColliderList>& tilesetColliders);
		
	static rapidxml::xml_node<>* GetChild(rapidxml::xml_node<>* inputNode, std::string sNodeFilter);
	static ColliderList GetColliders(rapidxml::xml_node<>* inputNode);

private:
	static SDL_Renderer* mCurrentRenderer;
};

// Returns a pointer to the tileset that the tileID belongs to. Returns nullptr if no match found.
static TilesetData* FindTilesetData(int tileID, std::vector<TilesetData>& tilesets)
{
	auto it = std::find_if(tilesets.rbegin(), tilesets.rend(), [tileID](const auto& set) {
		return set.firstID <= tileID;
	});
	if (it != tilesets.rend())
		return &(*it);

	return nullptr;
}