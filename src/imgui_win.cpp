// connection between clap_plugin_gui_win32 and imgui.
// plugin implementations should not need to touch this file.

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>

#include "imgui/imgui_internal.h" // so we can get the viewport associated with an ImGui window
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw/include/GLFW/glfw3.h"
#include "glfw/include/GLFW/glfw3native.h"

#include "main.h"


GLFWwindow *backend_wnd;
DWORD want_teardown;

struct ui_ctx_rec
{
  Plugin *plugin;
  HWND parent;
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
    RECT wr;
    GetWindowRect(rec->parent, &wr);

    ImGui::SetNextWindowPos(ImVec2(wr.left, wr.top));
    ImGui::SetNextWindowSize(ImVec2(wr.right-wr.left, wr.bottom-wr.top));
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
          HWND new_hwnd=(HWND)glfwGetWin32Window(glfw_win);
          if (new_hwnd) // assert
          {
            SetParent(new_hwnd, rec->parent);
            long style=GetWindowLong(new_hwnd, GWL_STYLE);
            style &= ~WS_POPUP;
            style |= WS_CHILDWINDOW;
            SetWindowLong(new_hwnd, GWL_STYLE, style);
            SetWindowPos(new_hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
            rec->did_parenting=1;
          }
          break;
        }
      }
    }
    rec=rec->next;
  }
}

void imgui__teardown()
{
  if (!backend_wnd) return;

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(backend_wnd); // will destroy backend hwnd and kill the timer
  backend_wnd=NULL;
  glfwTerminate();
}

void imgui__render_timer(HWND hwnd, UINT message, UINT_PTR caller, DWORD time)
{
  if (want_teardown > 0)
  {
    if (timeGetTime() > want_teardown) imgui__teardown();
  }
  else
  {
    imgui__do_render_pass();
  }
}

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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

    HWND backend_hwnd=(HWND)glfwGetWin32Window(backend_wnd);
    SetTimer(backend_hwnd, 1, 30, (TIMERPROC)imgui__render_timer);
  }

  ui_ctx_rec *new_rec=(ui_ctx_rec*)malloc(sizeof(ui_ctx_rec));
  memset(new_rec, 0, sizeof(ui_ctx_rec));
  new_rec->plugin=this;
  new_rec->parent=(HWND)parent;
  sprintf(new_rec->name, "%p", new_rec);
  m_ui_ctx=new_rec;

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

  if (!rec_list) want_teardown=timeGetTime()+1000;
}

void gui__on_plugin_destroy()
{
  if (!rec_list && backend_wnd) imgui__teardown();
}
