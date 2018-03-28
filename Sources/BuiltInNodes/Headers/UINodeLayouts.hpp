#ifndef UINODELAYOUTS_HPP
#define UINODELAYOUTS_HPP

#include "UINode.hpp"
#include "NodeUIEnvironment.hpp"
#include "NodeDrawingImage.hpp"

namespace BI
{

void DrawStatusHeaderWithSlotsLayout (	const NUIE::UINode& uiNode,
										NUIE::NodeUIDrawingEnvironment& env,
										NUIE::NodeDrawingImage& drawingImage);

void DrawHeaderWithSlotsAndButtonsLayout (	const NUIE::UINode& uiNode,
											const std::string& leftButtonId,
											const std::wstring& leftButtonText,
											const std::string& rightButtonId,
											const std::wstring& rightButtonText,
											NUIE::NodeUIDrawingEnvironment& env,
											NUIE::NodeDrawingImage& drawingImage);

}

#endif
