#pragma once

#include "pch.h"

static SDL_Texture* LoadTexture(const char* filename, SDL_Renderer* renderer)
{
	SDL_Texture* tex = nullptr;
	SDL_Surface* tempSurface = IMG_Load(filename);
	if (tempSurface == NULL)
	{
		std::cout << "Could not load textures." << std::endl;
	}
	else
	{
		tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
		if (tex == NULL)
		{
			std::cout << SDL_GetError() << std::endl;
		}
	}
	SDL_FreeSurface(tempSurface);
	return tex;
}

static void DestroyTexture(SDL_Texture* tex)
{
	SDL_DestroyTexture(tex);
}
