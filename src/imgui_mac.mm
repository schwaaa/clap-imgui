// connection between clap_plugin_gui_cocoa and imgui.
// plugin implementations should not need to touch this file.

#include <Cocoa/Cocoa.h>
#include <sys/time.h>

#include "imgui/imgui_internal.h" // so we can get the viewport associated with an ImGui window
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "glfw/include/GLFW/glfw3.h"
#include "glfw/include/GLFW/glfw3native.h"

#include "main.h"


@class imgui__timer;
imgui__timer *timer;
GLFWwindow *backend_wnd;
unsigned int want_teardown;

struct ui_ctx_rec
{
  Plugin *plugin;
  NSView *parent;
  char name[64];
  int did_parenting;
  ui_ctx_rec *next;
};
ui_ctx_rec *rec_list;

void imgui__do_render_pass()
{
  if (!backend_wnd) return;

 // glfwPollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ui_ctx_rec *rec=rec_list;
  while (rec)
  {
    NSRect vr = [rec->parent convertRect:[rec->parent bounds] toView:nil];
    NSRect wr = [[rec->parent window] convertRectToScreen:vr];
    wr.origin.y = CGDisplayBounds(CGMainDisplayID()).size.height-(wr.origin.y+wr.size.height);
    ImGui::SetNextWindowPos(ImVec2(wr.origin.x, wr.origin.y));
    ImGui::SetNextWindowSize(ImVec2(wr.size.width, wr.size.height));
    ImGui::Begin(rec->name, NULL,
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);
    rec->plugin->plugin_impl__draw();
    ImGui::End();

    rec=rec->next;
  }

  ImGui::Render();
  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();

  rec=rec_list;
  while (rec)
  {
    if (!rec->did_parenting)
    {
      extern ImGuiContext *GImGui;
      for (int i=0; i < GImGui->Windows.Size; ++i)
      {
        ImGuiWindow *w=GImGui->Windows[i];
        if (w->Name && !strcmp(w->Name, rec->name) &&
          w->Viewport && w->Viewport->PlatformWindowCreated)
        {
          GLFWwindow *glfw_win=(GLFWwindow*)w->Viewport->PlatformHandle;
          NSWindow *new_win=(NSWindow*)glfwGetCocoaWindow(glfw_win);
          if (new_win) // assert
          {
            [[rec->parent window] addChildWindow:new_win ordered:NSWindowAbove];
            rec->did_parenting=1;
          }
          break;
        }
      }
    }
    rec=rec->next;
  }
}

@interface imgui__timer : NSObject
{
@public
  NSTimer *timer;
}
-(void)on_timer:(id)sender;
@end

void imgui__teardown()
{
  if (!backend_wnd) return;

  [timer->timer invalidate];
  [timer release];

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(backend_wnd);
  backend_wnd=NULL;
  glfwTerminate();
}

unsigned int get_tick_count()
{
  struct timeval tm = {0};
  gettimeofday(&tm, NULL);
  return (unsigned int)(tm.tv_sec*1000 + tm.tv_usec/1000);
}

@implementation imgui__timer
-(void)on_timer:(id)sender
{
  if (want_teardown > 0)
  {
    if (get_tick_count() > want_teardown) imgui__teardown();
  }
  else
  {
    imgui__do_render_pass();
  }
}
@end

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool Plugin::gui__attach(void *parent)
{
  if (!parent) return false;
  if (m_ui_ctx) return true;

  want_teardown=0;
  if (!backend_wnd)
  {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    // invisible top level window
    backend_wnd=glfwCreateWindow(1, 1, "ImGui Backend", NULL, NULL);
    if (!backend_wnd) return false;

    glfwMakeContextCurrent(backend_wnd);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigViewportsNoAutoMerge=true;
    ImGui::GetIO().ConfigViewportsNoTaskBarIcon=true;
    ImGui::GetIO().ConfigViewportsNoDefaultParent=true;

    ImGui_ImplGlfw_InitForOpenGL(backend_wnd, true);
    ImGui_ImplOpenGL3_Init(NULL);

    timer = [imgui__timer new];
    timer->timer = [NSTimer scheduledTimerWithTimeInterval:0.030
      target:timer selector:@selector(on_timer:) userInfo:nil repeats:YES];
  }

  ui_ctx_rec *new_rec=(ui_ctx_rec*)malloc(sizeof(ui_ctx_rec));
  memset(new_rec, 0, sizeof(ui_ctx_rec));
  new_rec->plugin = this;
  new_rec->parent = (NSView*)parent;
  sprintf(new_rec->name, "CLAP ImGui %p", new_rec);
  m_ui_ctx = new_rec;

  if (rec_list) new_rec->next=rec_list;
  rec_list=new_rec;

  return true;
}

void Plugin::gui__destroy()
{
  if (!m_ui_ctx) return;

  ui_ctx_rec *old_rec=(ui_ctx_rec*)m_ui_ctx;
  m_ui_ctx=NULL;

  ui_ctx_rec *prev_rec=NULL, *rec=rec_list;
  while (rec)
  {
    if (rec == old_rec)
    {
      if (!prev_rec) rec_list=old_rec->next;
      else prev_rec->next=old_rec->next;
      break;
    }
    prev_rec=rec;
    rec=rec->next;
  }
  free(old_rec);

  if (!rec_list) want_teardown = get_tick_count()+1000;
}

void gui__on_plugin_destroy()
{
  if (!rec_list && backend_wnd) imgui__teardown();
}
