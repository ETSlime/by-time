#pragma once

#include "Scene/Item.h"

class Interactable : public Item {

public:

	Interactable(Transform transform);
	~Interactable();

	void Update(void) override;
	void Draw(void) override;

	void SetActive(bool active) override;

private:
	FBXLoader& m_fbxLoader = FBXLoader::get_instance();

	Collider m_triggerCollider;

};