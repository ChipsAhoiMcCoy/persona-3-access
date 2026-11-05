#pragma once
#include <string>
#include <vector>
#include "ue/ue.h"

namespace p3a::screens::umg {
// The visible text of a UMG widget tree, in tree order: UUserWidget -> WidgetTree.RootWidget, UPanelWidget.Slots ->
// UPanelSlot.Content, UContentWidget too; UTextBlock.Text (and URichTextBlock.Text) of widgets whose Visibility is not
// Collapsed/Hidden. Game thread.
std::vector<std::string> texts(ue::UObject* user_widget);
bool visible(ue::UObject* widget);
bool in_viewport(ue::UObject* user_widget);
}  // namespace p3a::screens::umg
