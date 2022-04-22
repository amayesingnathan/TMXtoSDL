#include "pch.h"

#include "TMXtoSDL.h"
#include "TexLoader.h"

using namespace rapidxml;

SDL_Renderer* TiledIO::mCurrentRenderer = nullptr;

enum class Attribute
{
    X,
    Y,
    Width,
    Height
};

static const std::unordered_map<std::string, Attribute> AttributeTable =
{
    {"x", Attribute::X},
    {"y", Attribute::Y},
    {"width", Attribute::Width},
    {"height", Attribute::Height}
};

xml_node<>* TiledIO::GetChild(xml_node<>* inputNode, std::string sNodeFilter)
{
    // cycles every child
    for (xml_node<>* nodeChild = inputNode->first_node(); nodeChild; nodeChild = nodeChild->next_sibling())
    {
        if (nodeChild->name() == sNodeFilter)
        {
            // returns the desired child
            return nodeChild;
        }
        xml_node<>* x = GetChild(nodeChild, sNodeFilter);
        if (x)
            return x;
    }
    return 0;
}

void TiledIO::GetLayers(xml_node<>* mapNode, std::vector<Layer>& layerList)
{
    int layerCount = 0;
    for (xml_node<>* layer = GetChild(mapNode, "layer"); layer; layer = layer->next_sibling())
    {
        layerCount++;
    }

    layerList.reserve(layerCount);

    xml_node<>* layerMetaData = GetChild(mapNode, "layer");

    int width = std::atoi(layerMetaData->first_attribute("width")->value());
    int height = std::atoi(layerMetaData->first_attribute("height")->value());

    Layer currLayer(width, height);

    std::string mapLine;
    std::string mapElement;

    for (xml_node<>* layer = GetChild(mapNode, "layer"); layer; layer = layer->next_sibling())
    {
        xml_node<>* layerData = GetChild(layer, "data");
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

void TiledIO::GetTileData(const std::filesystem::path& tileset, int firstID, std::unordered_map<int, TilesetData>& setData, std::unordered_map<int, ColliderList>& tilesetColliders)
{
    std::string tilesetTmx = "levels/l";

    //Remove .tsx extension and replace with .png
    std::filesystem::path tilesetPng = tileset.stem();
    tilesetPng += ".png";

    //Get initial node
    file<> xmlFile(tileset.string().c_str());
    xml_document<>* doc = new xml_document<>;
    doc->parse<0>(xmlFile.data());
    xml_node<>* parent = doc->first_node();

    TilesetData data;

    //Build data struct for this tileset 
    data.tilesetTex = LoadTexture(tilesetPng.string().c_str(), mCurrentRenderer);
    data.tilesetWidth = std::atoi(parent->first_attribute("columns")->value());
    data.tileWidth = std::atoi(parent->first_attribute("tilewidth")->value());
    data.tileHeight = std::atoi(parent->first_attribute("tileheight")->value());

    setData.emplace(firstID, data);

    //Build list of colliders for each tileID
    for (xml_node<>* tile = GetChild(parent, "tile"); tile; tile = tile->next_sibling())
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

void TiledIO::GetTileData(xml_node<>* tilesetNode, int firstID, const std::filesystem::path& pngPath, std::unordered_map<int, TilesetData>& setData, std::unordered_map<int, ColliderList>& tilesetColliders)
{
    int tileWidth = std::atoi(tilesetNode->first_attribute("tilewidth")->value());
    int tileHeight = std::atoi(tilesetNode->first_attribute("tileheight")->value());
    int tilesetWidth = std::atoi(tilesetNode->first_attribute("columns")->value());
    std::filesystem::path tilesetPath = pngPath;
    tilesetPath += std::filesystem::path(GetChild(tilesetNode, "image")->first_attribute("source")->value());
    SDL_Texture* tilesetTex = LoadTexture(tilesetPath.string().c_str(), mCurrentRenderer);

    setData.emplace(firstID, TilesetData{ tilesetTex, tileWidth, tileHeight, tilesetWidth });

    for (xml_node<>* tile = GetChild(tilesetNode, "tile"); tile; tile = tile->next_sibling())
    {
        int tileID = std::atoi(tile->first_attribute("id")->value());
        tilesetColliders.emplace(firstID + tileID, GetColliders(tile));
    }

}

void TiledIO::GetTilesets(xml_node<>* mapNode, const std::filesystem::path& lvlPath, std::unordered_map<int, TilesetData>& setData, std::unordered_map<int, ColliderList>& tilesetColliders)
{
    for (xml_node<>* tileset = GetChild(mapNode, "tileset"); std::string(tileset->name()) == "tileset"; tileset = tileset->next_sibling())
    {
        int firstGridID = std::atoi(tileset->first_attribute("firstgid")->value());

        //Some tilesets are included inline in file, some stored externally. 
        if (xml_attribute<>* source = tileset->first_attribute("source"))
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
}

ColliderList TiledIO::GetColliders(rapidxml::xml_node<>* inputNode)
{
    //Initialise vector to return
    ColliderList returnColliders;

    // cycles every collider in group
    for (xml_node<>* collider = GetChild(inputNode, "object"); collider; collider = collider->next_sibling())
    {
        int x = 0, y = 0, w = 0, h = 0;
        std::string attrName;

        // Cycles every attribute of the collider
        for (xml_attribute<>* nodeAttr = collider->first_attribute("x"); nodeAttr; nodeAttr = nodeAttr->next_attribute())
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

void TiledIO::OpenLevel(const std::filesystem::path& lvlPath, std::vector<Layer>& layerList, std::unordered_map<int, TilesetData>& tilesetData, std::unordered_map<int, ColliderList>& tilesetColliders, SDL_Renderer* renderer)
{
    if (!renderer && !mCurrentRenderer) return;
    if (renderer) mCurrentRenderer = renderer;

    std::filesystem::path lvlName = lvlPath.parent_path().filename();
    std::filesystem::path lvlLoc = lvlPath;
    lvlLoc /= lvlName;
    lvlLoc += ".tmx";

    file<> xmlFile(lvlLoc.string().c_str());
    xml_document<>* doc = new xml_document<>;
    doc->parse<0>(xmlFile.data());

    xml_node<>* mapNode = doc->first_node("map");

    GetLayers(mapNode, layerList);
    GetTilesets(mapNode, lvlPath, tilesetData, tilesetColliders);

    delete doc;
}
