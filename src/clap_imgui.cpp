#include <stdio.h>
#include <string.h>

#include "imgui/imgui.h"

#include "main.h"


bool Plugin::imgui__create()
{
  return true;
}

void Plugin::imgui__set_scale(double scale) { }

bool Plugin::imgui__get_size(uint32_t* width, uint32_t* height)
{
  *width=400;
  *height=400;
  return true;
}

bool Plugin::imgui__can_resize() { return false; }
void Plugin::imgui__round_size(uint32_t* width, uint32_t* height) { }
bool Plugin::imgui__set_size(uint32_t width, uint32_t height) { return false; }

void Plugin::imgui__show()
{
}

void Plugin::imgui__hide()
{
}


