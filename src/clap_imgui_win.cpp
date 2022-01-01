#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw/include/GLFW/glfw3.h"
#include "glfw/include/GLFW/glfw3native.h"

#include "main.h"


GLFWwindow *backend_wnd;
int backend_refcnt;
struct ui_ctx_rec
{
  Plugin *plugin;
  HWND parent;
  char name[64];
  ImGuiViewport *viewport;
  ui_ctx_rec *next;
};
ui_ctx_rec *rec_list;

void imgui__do_render_pass(ui_ctx_rec *new_rec)
{
  if (!backend_wnd || !rec_list) return;

  //glfwPollEvents();

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

  //ImGui::ShowDemoWindow();

  ImGui::Render();
  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();

  if (new_rec)
  {
    if (ImGui::GetPlatformIO().Viewports.Size) // assert
    {
      new_rec->viewport=ImGui::GetPlatformIO().Viewports.back();
      GLFWwindow *glfw_win=(GLFWwindow*)new_rec->viewport->PlatformHandle;
      HWND new_hwnd=(HWND)glfwGetWin32Window(glfw_win);
      if (glfw_win && new_hwnd) // assert
      {
        SetParent(new_hwnd, new_rec->parent);
        long style=GetWindowLong(new_hwnd, GWL_STYLE);
        style &= ~WS_POPUP;
        style |= WS_CHILDWINDOW;
        SetWindowLong(new_hwnd, GWL_STYLE, style);
      }
    }
  }

  //int w, h;
  //glfwGetFramebufferSize(backend_wnd, &w, &h);
  //glViewport(0, 0, w, h);
  //glClearColor(0.0, 0.0, 0.0, 1.0);
  //glClear(GL_COLOR_BUFFER_BIT);
  //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  //glfwSwapBuffers(backend_wnd);
}

void imgui__on_timer(HWND hwnd, UINT message, UINT_PTR caller, DWORD time)
{
  imgui__do_render_pass(NULL);
}

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool Plugin::imgui__attach(void *parent)
{
  if (!parent) return false;
  if (m_ui_ctx) return true;

  if (!backend_refcnt)
  {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    // invisible top level window
    backend_wnd=glfwCreateWindow(1000, 1000, "ImGui Backend", NULL, NULL);
    if (!backend_wnd) return false;

    glfwMakeContextCurrent(backend_wnd);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigViewportsNoAutoMerge=true;
    ImGui::GetIO().ConfigViewportsNoTaskBarIcon=true;
    ImGui::GetIO().ConfigViewportsNoDefaultParent=true;

    ImGui_ImplGlfw_InitForOpenGL(backend_wnd, true);
    ImGui_ImplOpenGL3_Init(NULL);

    HWND backend_hwnd=(HWND)glfwGetWin32Window(backend_wnd);
    SetTimer(backend_hwnd, 1, 30, (TIMERPROC)imgui__on_timer);
  }
  ++backend_refcnt;

  ui_ctx_rec *rec=(ui_ctx_rec*)malloc(sizeof(ui_ctx_rec));
  memset(rec, 0, sizeof(ui_ctx_rec));
  rec->plugin=this;
  rec->parent=(HWND)parent;
  sprintf(rec->name, "ImGui %p", this);
  m_ui_ctx=rec;

  if (rec_list) rec->next=rec_list;
  rec_list=rec;

  imgui__do_render_pass(rec);

  return true;
}

void Plugin::imgui__destroy()
{
  if (!m_ui_ctx) return;

  ui_ctx_rec *rec=(ui_ctx_rec*)m_ui_ctx;
  m_ui_ctx=NULL;

  ui_ctx_rec *prev_rec=NULL, *cur_rec=rec_list;
  while (cur_rec)
  {
    if (cur_rec == rec)
    {
      if (!prev_rec) rec_list=rec->next;
      else prev_rec->next=rec->next;
      break;
    }
  }
  free(rec);

  if (--backend_refcnt > 0)
  {
    imgui__do_render_pass(NULL); // clear out the old viewport
  }
  else
  {
    HWND backend_hwnd=(HWND)glfwGetWin32Window(backend_wnd);
    KillTimer(backend_hwnd, 1);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(backend_wnd);
    backend_wnd=NULL;
    glfwTerminate();
  }
}

