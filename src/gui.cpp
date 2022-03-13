// generic ui functions.
// plugin implementations that use imgui should not need to touch this file.

#include <stdio.h>
#include <string.h>

#include "main.h"


bool Plugin::gui__create(const char *api, bool is_floating)
{
  return gui__is_api_supported(api, is_floating);
}

bool Plugin::gui__set_scale(double scale)
{
  return false;
}

bool Plugin::gui__get_size(uint32_t *width, uint32_t *height)
{
  if (m_w > 0 && m_h > 0) { *width=m_w; *height=m_h; }
  else plugin_impl__get_preferred_size(width, height);
  return true;
}

bool Plugin::gui__can_resize()
{
  return true;
}

bool Plugin::gui__adjust_size(uint32_t *width, uint32_t *height)
{
  return true;
}

bool Plugin::gui__set_size(uint32_t width, uint32_t height)
{
  // imgui should respond dynamically to the host window size changing
  m_w=width;
  m_h=height;
  return true;
}

bool Plugin::gui__set_transient(const clap_window *window)
{
  return false;
}

void Plugin::gui__suggest_title(const char *title)
{
}

bool Plugin::gui__show()
{
  return true;
}

bool Plugin::gui__hide()
{
  return true;
}
