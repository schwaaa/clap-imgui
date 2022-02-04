// imgui helpers for x11.
// plugin implementations should not need to touch this file.

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "main.h"

#include "glfw/include/GLFW/glfw3.h"
#include "glfw/include/GLFW/glfw3native.h"

bool imgui__attach(Plugin *plugin, void *native_display, void *native_window);

bool gui::attach_lin(const clap_plugin *plugin, const char *display_name, unsigned long parent)
{
  if (!display_name || !parent) return false;
  Display *display = XOpenDisplay(display_name);
  if (!display) return false;

  return imgui__attach((Plugin*)plugin->plugin_data, display, (void*)parent);
}

bool gui::attach_mac(const clap_plugin *plugin, void *parent) { return false; }
bool gui::attach_win(const clap_plugin *plugin, void *parent) { return false; }
  
void imgui__get_native_window_position(void *native_display, void *native_window,
  int *x, int *y, int *w, int *h)
{
  XWindowAttributes xwa;
  XGetWindowAttributes((Display*)native_display, (Window)native_window, &xwa);
  *x = xwa.x;
  *y = xwa.y;
  *w = xwa.width;
  *h = xwa.height;
}

bool imgui__set_native_parent(void *native_display, void *native_window, GLFWwindow *glfw_win)
{
  unsigned long win = glfwGetX11Window(glfw_win);
  if (win)
  {
    XReparentWindow((Display*)native_display, win, (Window)native_window, 0, 0); 
    return true;
  }
  return false;
}

unsigned int get_tick_count()
{
  struct timeval tm = {0};
  gettimeofday(&tm, NULL);
  return (unsigned int)(tm.tv_sec*1000 + tm.tv_usec/1000);
}
