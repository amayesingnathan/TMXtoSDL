#pragma once

#include <vector>

class Layer
{
public:
	Layer(size_t width, size_t height)
		: mWidth(width), mHeight(height)
	{ mElements.reserve(width * height); }

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

	void push_back(int element) { mElements.push_back(element); }

	void clear() { mElements.clear(); }

	int& operator()(size_t x, size_t y) { return mElements[(mWidth * y) + x]; }
	const int& operator()(size_t x, size_t y) const { return mElements[(mWidth * y) + x]; }

private:
	std::vector<int> mElements;
	size_t mWidth;
	size_t mHeight;
};