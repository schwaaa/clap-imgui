// platform independent connection between clap and imgui.
// plugin implementations should not need to touch this file.

#include "main.h"

#include "glfw/include/GLFW/glfw3.h"

#include "imgui/imgui_internal.h" // so we can get the viewport associated with an ImGui window
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

void imgui__get_native_window_position(void *native_display, void *native_window,
  int *x, int *y, int *w, int *h);
bool imgui__set_native_parent(void *native_display, void *native_window, GLFWwindow *glfw_win);
unsigned int get_tick_count();

extern const clap_host *g_clap_host;

#define HOST_TIMER_MS 30
unsigned int host_timer_id;
clap_host_timer_support *host_timer_support;

GLFWwindow *backend_wnd;
unsigned int want_teardown;

struct ui_ctx_rec
{
  Plugin *plugin;
  void *native_display; // only used for x11
  void *native_window;
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
    int x, y, w, h;
    imgui__get_native_window_position(rec->native_display, rec->native_window, &x, &y, &w, &h);
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
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
          if (imgui__set_native_parent(rec->native_display, rec->native_window, glfw_win))
          {
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

  if (host_timer_support) host_timer_support->unregister_timer(g_clap_host, host_timer_id);
  host_timer_support=NULL;
  host_timer_id=0;

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(backend_wnd); // will destroy backend hwnd and kill the timer
  backend_wnd=NULL;
  glfwTerminate();
}

void imgui__on_timer(const clap_plugin *plugin, unsigned int timer_id)
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

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool imgui__attach(Plugin *plugin, void *native_display, void *native_window)
{
  if (!plugin || !native_window) return false;
  if (plugin->m_ui_ctx) return true;

  want_teardown=0;
  if (!backend_wnd)
  {
    // for simplicity and portability, require clap_host_timer_support.
    // this could be implemented instead on win and mac via a platform-dependent main thread system timer,
    // but linux would have to spawn a thread.
    host_timer_support =
      (clap_host_timer_support*)g_clap_host->get_extension(g_clap_host, CLAP_EXT_TIMER_SUPPORT);
    if (!host_timer_support) return false;

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

    host_timer_support->register_timer(g_clap_host, HOST_TIMER_MS, &host_timer_id);
  }

  ui_ctx_rec *new_rec=(ui_ctx_rec*)malloc(sizeof(ui_ctx_rec));
  memset(new_rec, 0, sizeof(ui_ctx_rec));
  new_rec->plugin = plugin;
  new_rec->native_display = native_display;
  new_rec->native_window = native_window;
  sprintf(new_rec->name, "%p", new_rec);
  plugin->m_ui_ctx=new_rec;

  if (rec_list) new_rec->next=rec_list;
  rec_list=new_rec;

  return true;
}

void gui__destroy(Plugin *plugin, bool is_plugin_destroy)
{
  if (plugin->m_ui_ctx)
  {
    ui_ctx_rec *old_rec=(ui_ctx_rec*)plugin->m_ui_ctx;
    plugin->m_ui_ctx=NULL;

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
  }

  if (!rec_list)
  {
    if (is_plugin_destroy) imgui__teardown();
    else want_teardown = get_tick_count()+1000;
  }
}

clap_plugin_timer_support gui__timer_support =
{
  imgui__on_timer
};
