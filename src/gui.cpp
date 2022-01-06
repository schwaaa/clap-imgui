// generic ui functions.
// plugin implementations that use imgui should not need to touch this file.

#include <stdio.h>
#include <string.h>

#include "main.h"


bool Plugin::gui__create() { return true; }
void Plugin::gui__set_scale(double scale) { }

bool Plugin::gui__get_size(uint32_t *width, uint32_t *height)
{
  if (m_w > 0 && m_h > 0) { *width=m_w; *height=m_h; }
  else plugin_impl__get_preferred_size(width, height);
  return true;
}

bool Plugin::gui__set_size(uint32_t width, uint32_t height)
{
  // todo implement
  m_w=width;
  m_h=height;
  return true;
}

bool Plugin::gui__can_resize() { return true; }
void Plugin::gui__round_size(uint32_t *width, uint32_t *height) { }

void Plugin::gui__show() { }
void Plugin::gui__hide() { }


