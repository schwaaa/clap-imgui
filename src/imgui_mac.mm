// imgui helpers for mac.
// plugin implementations should not need to touch this file.

#include <Cocoa/Cocoa.h>
#include <sys/time.h>

#include "main.h"

#include "glfw/include/GLFW/glfw3.h"
#include "glfw/include/GLFW/glfw3native.h"

bool imgui__attach(Plugin *plugin, void *native_display, void *native_window);

bool gui::attach_mac(const clap_plugin *plugin, void *parent)
{
  return imgui__attach((Plugin*)plugin->plugin_data, NULL, parent);
}

bool gui::attach_win(const clap_plugin *plugin, void *parent) { return false; }
bool gui::attach_lin(const clap_plugin *plugin, const char *display_name, unsigned long parent) { return false; }

void imgui__get_native_window_position(void *native_display, void *native_window,
  int *x, int *y, int *w, int *h)
{
  NSView *vw = (NSView*)native_window;
  NSRect vr = [vw convertRect:[vw bounds] toView:nil];
  NSRect wr = [[vw window] convertRectToScreen:vr];
  wr.origin.y = CGDisplayBounds(CGMainDisplayID()).size.height-(wr.origin.y+wr.size.height);
  *x = wr.origin.x;
  *y = wr.origin.y;
  *w = wr.size.width;
  *h = wr.size.height;
}

bool imgui__set_native_parent(void *native_display, void *native_window, GLFWwindow *glfw_win)
{
  NSWindow *win=(NSWindow*)glfwGetCocoaWindow(glfw_win);
  if (win)
  {
    NSView *vw = (NSView*)native_window;
    [[vw window] addChildWindow:win ordered:NSWindowAbove];
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

