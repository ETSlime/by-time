#pragma once

#include "Scene/Item.h"

#define SWORD_SIZE XMFLOAT3(90.f, 90.f, 90.f)

class Sword : public Item {

public: 

	Sword(Transform transform);
	~Sword();

	void Update(void) override;
	void Draw(void) override;

private:
	FBXLoader& m_fbxLoader = FBXLoader::get_instance();

};