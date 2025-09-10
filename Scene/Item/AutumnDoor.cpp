#include "AutumnDoor.h"
#include "Scene/Ground.h"

void AutumnDoor::Interact()
{
	if (!m_opened) {

		if (Ground::get_instance().GetCurrentScene() == SceneID::SCENE_6) {

			m_opened = true;

			Transform t = GetTransform();
			//t.pos.x += m_side == DoorSide::Left?100:-100;
			t.rot.y += m_side == DoorSide::Left ? 1.5f : -1.5f;

			SetTransform(t);

			if (m_linkedDoor != nullptr) {
			
				m_linkedDoor->Interact();

			}
		}

		
	}
}
