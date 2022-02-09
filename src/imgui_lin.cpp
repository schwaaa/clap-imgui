// imgui helpers for x11.
// plugin implementations should not need to touch this file.

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "main.h"

#include "glfw/include/GLFW/glfw3.h"
#include "glfw/include/GLFW/glfw3native.h"

bool imgui__attach(Plugin *plugin, void *native_display, void *native_window);

bool gui__attach_lin(const clap_plugin *plugin, const char *display_name, unsigned long parent)
{
  if (!display_name || !parent) return false;
  Display *display = XOpenDisplay(display_name);
  if (!display) return false;

  return imgui__attach((Plugin*)plugin->plugin_data, display, (void*)parent);
}

bool gui__attach_mac(const clap_plugin *plugin, void *parent) { return false; }
bool gui__attach_win(const clap_plugin *plugin, void *parent) { return false; }
  
void get_native_window_position(void *native_display, void *native_window,
  int *x, int *y, int *w, int *h)
{
  Display *xdisp = (Display*)native_display;
  Window xwin = (Window)native_window;

  XWindowAttributes xwa;
  XGetWindowAttributes(xdisp, xwin, &xwa);

  Window xroot = DefaultRootWindow(xdisp), xchild = 0;
  if (xroot) XTranslateCoordinates(xdisp, xwin, xroot, 0, 0, &xwa.x, &xwa.y, &xchild);

  *x = xwa.x;
  *y = xwa.y;
  *w = xwa.width;
  *h = xwa.height;
}

bool set_native_parent(void *native_display, void *native_window, GLFWwindow *glfw_win)
{
  unsigned long win = glfwGetX11Window(glfw_win);
  if (win)
  {
    XReparentWindow((Display*)native_display, win, (Window)native_window, 0, 0); 
    return true;
  }
  return false;
}

extern clap_host *g_clap_host;
unsigned int timer_id;

bool create_timer(unsigned int ms)
{
  clap_host_timer_support *timer_support =
    (clap_host_timer_support*)g_clap_host->get_extension(g_clap_host, CLAP_EXT_TIMER_SUPPORT);
  return timer_support && timer_support->register_timer(g_clap_host, ms, &timer_id);
}

void destroy_timer()
{
  clap_host_timer_support *timer_support =
    (clap_host_timer_support*)g_clap_host->get_extension(g_clap_host, CLAP_EXT_TIMER_SUPPORT);
  if (timer_support) timer_support->unregister_timer(g_clap_host, timer_id);
  timer_id=0;
}

unsigned int get_tick_count()
{
  struct timeval tm = {0};
  gettimeofday(&tm, NULL);
  return (unsigned int)(tm.tv_sec*1000 + tm.tv_usec/1000);
}
