#include "NUIE_InputEventHandler.hpp"

namespace NUIE
{

static const std::unordered_set<ModifierKeyCode> noKeys;
ModifierKeys EmptyModifierKeys (noKeys);
Key InvalidKey;

ModifierKeys::ModifierKeys (const std::unordered_set<ModifierKeyCode>& keys) :
	keys (keys)
{

}

ModifierKeys::~ModifierKeys ()
{

}


bool ModifierKeys::Contains (ModifierKeyCode keyCode) const
{
	return keys.find (keyCode) != keys.end ();
}

Key::Key () :
	pressedKeyCode (PressedKeyCode::Undefined)
{

}

Key::Key (PressedKeyCode pressedKeyCode) :
	pressedKeyCode (pressedKeyCode)
{

}

bool Key::IsValid () const
{
	return pressedKeyCode != PressedKeyCode::Undefined;
}

PressedKeyCode Key::GetKeyCode () const
{
	DBGASSERT (pressedKeyCode != PressedKeyCode::Undefined);
	return pressedKeyCode;
}

InputEventHandler::InputEventHandler ()
{

}

InputEventHandler::~InputEventHandler ()
{

}

MouseEventTranslator::MouseEventTranslator (InputEventHandler& handler) :
	handler (handler)
{

}

MouseEventTranslator::~MouseEventTranslator ()
{

}

void MouseEventTranslator::OnMouseDown (NodeUIEnvironment&, const ModifierKeys&, MouseButton mouseButton, const Point& position)
{
	downMouseButtons.insert ({ mouseButton, position });
}

void MouseEventTranslator::OnMouseUp (NodeUIEnvironment& env, const ModifierKeys& modifierKeys, MouseButton mouseButton, const Point& position)
{
	if (movingMouseButtons.find (mouseButton) != movingMouseButtons.end ()) {
		movingMouseButtons.erase (mouseButton);
		handler.HandleMouseDragStop (env, modifierKeys, mouseButton, position);
	}

	if (downMouseButtons.find (mouseButton) != downMouseButtons.end ()) {
		downMouseButtons.erase (mouseButton);
		handler.HandleMouseClick (env, modifierKeys, mouseButton, position);
	}
}

void MouseEventTranslator::OnMouseMove (NodeUIEnvironment& env, const ModifierKeys& modifierKeys, const Point& position)
{
	std::unordered_set<MouseButton> downButtonsMoved;
	for (const auto& it : downMouseButtons) {
		if (it.second.DistanceTo (position) > env.GetMouseMoveMinOffset ()) {
			movingMouseButtons.insert (it.first);
			handler.HandleMouseDragStart (env, modifierKeys, it.first, it.second);
			downButtonsMoved.insert (it.first);
		}
	}
	for (MouseButton downButtonMoved : downButtonsMoved) {
		downMouseButtons.erase (downButtonMoved);
	}

	if (!movingMouseButtons.empty ()) {
		handler.HandleMouseDrag (env, modifierKeys, position);
	}
}

void MouseEventTranslator::OnMouseWheel (NodeUIEnvironment& env, const ModifierKeys& modifierKeys, MouseWheelRotation rotation, const Point& position)
{
	handler.HandleMouseWheel (env, modifierKeys, rotation, position);
}

void MouseEventTranslator::OnMouseDoubleClick (NodeUIEnvironment& env, const ModifierKeys& modifierKeys, MouseButton mouseButton, const Point& position)
{
	// TODO: this is a hack
	handler.HandleMouseClick (env, modifierKeys, mouseButton, position);
}

}