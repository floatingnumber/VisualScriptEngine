#include "WAS_HwndEventHandler.hpp"
#include "WAS_ParameterDialog.hpp"
#include "WAS_WindowsAppUtils.hpp"

namespace WAS
{

HwndEventHandler::HwndEventHandler () :
	NUIE::EventHandler (),
	hwnd (nullptr)
{

}

HwndEventHandler::~HwndEventHandler ()
{

}

void HwndEventHandler::Init (HWND windowHandle)
{
	hwnd = windowHandle;
}

NUIE::MenuCommandPtr HwndEventHandler::OnContextMenu (const NUIE::Point& position, const NUIE::MenuCommandStructure& commands)
{
	return WAS::SelectCommandFromContextMenu (hwnd, position, commands);
}

NUIE::MenuCommandPtr HwndEventHandler::OnContextMenu (const NUIE::Point& position, const NUIE::UINodePtr&, const NUIE::MenuCommandStructure& commands)
{
	return WAS::SelectCommandFromContextMenu (hwnd, position, commands);
}

NUIE::MenuCommandPtr HwndEventHandler::OnContextMenu (const NUIE::Point& position, const NUIE::UIOutputSlotConstPtr&, const NUIE::MenuCommandStructure& commands)
{
	return WAS::SelectCommandFromContextMenu (hwnd, position, commands);
}

NUIE::MenuCommandPtr HwndEventHandler::OnContextMenu (const NUIE::Point& position, const NUIE::UIInputSlotConstPtr&, const NUIE::MenuCommandStructure& commands)
{
	return WAS::SelectCommandFromContextMenu (hwnd, position, commands);
}

NUIE::MenuCommandPtr HwndEventHandler::OnContextMenu (const NUIE::Point& position, const NUIE::UINodeGroupPtr&, const NUIE::MenuCommandStructure& commands)
{
	return WAS::SelectCommandFromContextMenu (hwnd, position, commands);
}

void HwndEventHandler::OnDoubleClick (const NUIE::Point&, NUIE::MouseButton)
{

}

bool HwndEventHandler::OnParameterSettings (NUIE::ParameterInterfacePtr paramAccessor, const NUIE::UINodePtr&)
{
	WAS::ParameterDialog paramDialog (paramAccessor);
	return paramDialog.Show (hwnd, 50, 50);
}

bool HwndEventHandler::OnParameterSettings (NUIE::ParameterInterfacePtr paramAccessor, const NUIE::UINodeGroupPtr&)
{
	WAS::ParameterDialog paramDialog (paramAccessor);
	return paramDialog.Show (hwnd, 50, 50);
}

}
