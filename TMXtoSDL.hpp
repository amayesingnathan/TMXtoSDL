#pragma once

#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <filesystem>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

#include "SDL.h"
#include "SDL_image.h"

namespace TMXtoSDL
{

    using Collider = SDL_Rect;
    using ColliderList = std::vector<Collider>;

    /// 
    ///  SDL TEXTURE FUNCTIONS
    /// 

    static SDL_Texture* LoadTexture(const char* filename, SDL_Renderer* renderer)
    {
        SDL_Texture* tex = nullptr;
		
        SDL_Surface* tempSurface = IMG_Load(filename);
        if (!tempSurface) { std::cout << "Could not load textures." << std::endl; return nullptr; }
		
		tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
		if (!tex) { std::cout << SDL_GetError() << std::endl; SDL_FreeSurface(tempSurface); return nullptr; }
		
        SDL_FreeSurface(tempSurface);
        return tex;
    }

    static void DestroyTexture(SDL_Texture* tex)
    {
        SDL_DestroyTexture(tex);
    }


    /// 
    ///  LAYER CLASS CONTAINING TILE IDS
    ///

    class Layer
    {
    public:
        Layer(size_t width, size_t height)
            : mWidth(width), mHeight(height)
        {
            mElements.reserve(width * height);
        }

        // This function clears the layer of any existing values
        void resize(size_t width, size_t height)
        {
            mElements.clear();
            mElements.reserve(width * height);

            mWidth = width;
            mHeight = height;
        }

        size_t getWidth() const { return mWidth; }
        size_t getHeight() const { return mHeight; }

        std::vector<int> getRow(size_t row)
        {
            const auto start = mElements.begin() + (row * mWidth);
            return std::vector<int>(start, start + mWidth);
        }

        void push_back(int element) { mElements.push_back(element); }

        void clear() { mElements.clear(); }

        int& operator()(size_t x, size_t y) { return mElements[(mWidth * y) + x]; }
        const int& operator()(size_t x, size_t y) const { return mElements[(mWidth * y) + x]; }

    private:
        std::vector<int> mElements;
        size_t mWidth;
        size_t mHeight;
    };


    /// 
    ///  TILESET DATA CONTAINING TEXTURE AND TILE DATA
    /// 

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


    /// 
    ///  XML ATTRIBUTE ENUM
    /// 

    enum class Attribute
    {
        X,
        Y,
        Width,
        Height
    };


    /// 
    ///  ATTRIBUTE STRING TO ENUM LOOKUP TABLE
    /// 

    static const std::unordered_map<std::string, Attribute> AttributeTable =
    {
        {"x", Attribute::X},
        {"y", Attribute::Y},
        {"width", Attribute::Width},
        {"height", Attribute::Height}
    };

    /// 
    ///  FILE IO INTERFACE. EXTRACTS TILEMAP DATA FROM .TMX FILE
    /// 

    class IO
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

    rapidxml::xml_node<>* IO::GetChild(rapidxml::xml_node<>* inputNode, std::string sNodeFilter)
    {
        // cycles every child
        for (rapidxml::xml_node<>* nodeChild = inputNode->first_node(); nodeChild; nodeChild = nodeChild->next_sibling())
        {
            if (nodeChild->name() == sNodeFilter)
            {
                // returns the desired child
                return nodeChild;
            }
            rapidxml::xml_node<>* x = GetChild(nodeChild, sNodeFilter);
            if (x)
                return x;
        }
        return 0;
    }

    void IO::GetLayers(rapidxml::xml_node<>* mapNode, std::vector<Layer>& layerList)
    {
        int layerCount = 0;
        for (rapidxml::xml_node<>* layer = GetChild(mapNode, "layer"); layer; layer = layer->next_sibling())
        {
            layerCount++;
        }

        layerList.reserve(layerCount);

        rapidxml::xml_node<>* layerMetaData = GetChild(mapNode, "layer");

        int width = std::atoi(layerMetaData->first_attribute("width")->value());
        int height = std::atoi(layerMetaData->first_attribute("height")->value());

        Layer currLayer(width, height);

        std::string mapLine;
        std::string mapElement;

        for (rapidxml::xml_node<>* layer = GetChild(mapNode, "layer"); layer; layer = layer->next_sibling())
        {
            rapidxml::xml_node<>* layerData = GetChild(layer, "data");
            std::istringstream mapStream(layerData->value());

            //Data begins with CRLF so read this first to ignore it.
            std::getline(mapStream, mapLine);

            while (std::getline(mapStream, mapLine))
            {
                std::istringstream lineStream(mapLine);
                while (std::getline(lineStream, mapElement, ','))
                {
                    if (mapElement != "\r" && mapElement != "\n")
                    {
                        currLayer.push_back(std::stoi(mapElement));
                    }
                }
            }
            layerList.emplace_back(std::move(currLayer));
            currLayer.clear();
        }
    }

    void IO::GetTileData(const std::filesystem::path& tileset, int firstID, std::vector<TilesetData>& setData, std::map<int, ColliderList>& tilesetColliders)
    {
        std::string tilesetTmx = "levels/l";

        //Remove .tsx extension and replace with .png
        std::filesystem::path tilesetPng = tileset.stem();
        tilesetPng += ".png";

        //Get initial node
        rapidxml::file<> xmlFile(tileset.string().c_str());
        rapidxml::xml_document<>* doc = new rapidxml::xml_document<>;
        doc->parse<0>(xmlFile.data());
        rapidxml::xml_node<>* parent = doc->first_node();

        //Build data struct for this tileset 
        SDL_Texture* tilesetTex = LoadTexture(tilesetPng.string().c_str(), mCurrentRenderer);
        int tileWidth = std::atoi(parent->first_attribute("tilewidth")->value());
        int tileHeight = std::atoi(parent->first_attribute("tileheight")->value());
        int tilesetWidth = std::atoi(parent->first_attribute("columns")->value());

        setData.emplace_back(firstID, tilesetTex, tileWidth, tileHeight, tilesetWidth);

        //Build list of colliders for each tileID
        for (rapidxml::xml_node<>* tile = GetChild(parent, "tile"); tile; tile = tile->next_sibling())
        {
            if (GetChild(tile, "objectgroup"))
            {
                //Add key value pair for this tile ID
                int tileID = std::atoi(tile->first_attribute()->value());
                tilesetColliders.emplace(firstID + tileID, GetColliders(tile));
            }
        }

        delete doc;
        doc = nullptr;
    }

    void IO::GetTileData(rapidxml::xml_node<>* tilesetNode, int firstID, const std::filesystem::path& pngPath, std::vector<TilesetData>& setData, std::map<int, ColliderList>& tilesetColliders)
    {
        int tileWidth = std::atoi(tilesetNode->first_attribute("tilewidth")->value());
        int tileHeight = std::atoi(tilesetNode->first_attribute("tileheight")->value());
        int tilesetWidth = std::atoi(tilesetNode->first_attribute("columns")->value());
        std::filesystem::path tilesetPath = pngPath;
        tilesetPath += std::filesystem::path(GetChild(tilesetNode, "image")->first_attribute("source")->value());
        SDL_Texture* tilesetTex = LoadTexture(tilesetPath.string().c_str(), mCurrentRenderer);

        setData.emplace_back(firstID, tilesetTex, tileWidth, tileHeight, tilesetWidth);

        for (rapidxml::xml_node<>* tile = GetChild(tilesetNode, "tile"); tile; tile = tile->next_sibling())
        {
            int tileID = std::atoi(tile->first_attribute("id")->value());
            tilesetColliders.emplace(firstID + tileID, GetColliders(tile));
        }

    }

    void IO::GetTilesets(rapidxml::xml_node<>* mapNode, const std::filesystem::path& lvlPath, std::vector<TilesetData>& setData, std::map<int, ColliderList>& tilesetColliders)
    {
        for (rapidxml::xml_node<>* tileset = GetChild(mapNode, "tileset"); std::string(tileset->name()) == "tileset"; tileset = tileset->next_sibling())
        {
            int firstGridID = std::atoi(tileset->first_attribute("firstgid")->value());

            //Some tilesets are included inline in file, some stored externally. 
            if (rapidxml::xml_attribute<>* source = tileset->first_attribute("source"))
            {
                std::filesystem::path tilesetPath = lvlPath;
                tilesetPath += std::filesystem::path(source->value());
                GetTileData(tilesetPath, firstGridID, setData, tilesetColliders);
            }
            else
            {
                GetTileData(tileset, firstGridID, lvlPath, setData, tilesetColliders);
            }
        }

        std::sort(setData.begin(), setData.end());
    }

    ColliderList IO::GetColliders(rapidxml::xml_node<>* inputNode)
    {
        //Initialise vector to return
        ColliderList returnColliders;

        // cycles every collider in group
        for (rapidxml::xml_node<>* collider = GetChild(inputNode, "object"); collider; collider = collider->next_sibling())
        {
            int x = 0, y = 0, w = 0, h = 0;
            std::string attrName;

            // Cycles every attribute of the collider
            for (rapidxml::xml_attribute<>* nodeAttr = collider->first_attribute("x"); nodeAttr; nodeAttr = nodeAttr->next_attribute())
            {
                attrName = nodeAttr->name();
                double fAttr = std::atof(nodeAttr->value());
                int attr = static_cast<int>(round(fAttr));

                if (AttributeTable.count(attrName) == 0) continue;

                switch (AttributeTable.at(attrName))
                {
                case Attribute::X:
                    x = attr;
                    break;

                case Attribute::Y:
                    y = attr;
                    break;

                case Attribute::Width:
                    w = attr;
                    break;

                case Attribute::Height:
                    h = attr;
                    break;

                default:
                    break;
                }
            }
            returnColliders.emplace_back(x, y, w, h);
        }
        return returnColliders;
    }

    void IO::OpenLevel(const std::filesystem::path& lvlPath, std::vector<Layer>& layerList, std::vector<TilesetData>& tilesetData, std::map<int, ColliderList>& tilesetColliders, SDL_Renderer* renderer)
    {
        if (!renderer && !mCurrentRenderer) return;
        if (renderer) mCurrentRenderer = renderer;

        std::filesystem::path lvlName = lvlPath.parent_path().filename();
        std::filesystem::path lvlLoc = lvlPath;
        lvlLoc /= lvlName;
        lvlLoc += ".tmx";

        rapidxml::file<> xmlFile(lvlLoc.string().c_str());
        rapidxml::xml_document<>* doc = new rapidxml::xml_document<>;
        doc->parse<0>(xmlFile.data());

        rapidxml::xml_node<>* mapNode = doc->first_node("map");

        GetLayers(mapNode, layerList);
        GetTilesets(mapNode, lvlPath, tilesetData, tilesetColliders);

        delete doc;
    }

    // Returns a pointer to the tileset that the tileID belongs to. Returns nullptr if no match found.
    static const TilesetData* FindTilesetData(int tileID, const std::vector<TilesetData>& tilesets)
    {
        auto it = std::find_if(tilesets.rbegin(), tilesets.rend(), [tileID](const auto& set) {
            return set.firstID <= tileID;
            });
        if (it != tilesets.rend())
            return &(*it);

        return nullptr;
    }

    static SDL_Rect GetSrcRect(int tileID, const std::vector<TilesetData>& tilesets)
    {
        return GetSrcRect(tileID, FindTilesetData(tileID, tilesets));
    }

    static SDL_Rect GetSrcRect(int tileID, const TilesetData* tileset)
    {
        if (!tileset) return {};

        int index = tileID - tileset->firstID;
        int row = index / tileset->tilesetWidth;
        int column = index % tileset->tilesetWidth;
        int x = column * tileset->tileWidth;
        int y = row * tileset->tileHeight;

        return { x, y, tileset->tilesetWidth, tileset->tileHeight };
    }
}
