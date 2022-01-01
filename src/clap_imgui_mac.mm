#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include "imgui/backends/imgui_impl_osx.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_metal.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "glfw/include/GLFW/glfw3.h"
#include "glfw/include/GLFW/glfw3native.h"

#include "main.h"


@interface ui_ctx_rec : NSObject
{
@public
  Plugin *plugin;
  GLFWwindow *glfw_win;
  id <MTLCommandQueue> cmd_q;
  MTLRenderPassDescriptor *render_pass;
  NSTimer *timer;
}
-(void)imgui__on_timer:(id)sender;
@end

@implementation ui_ctx_rec
-(void)imgui__on_timer:(id)sender
{
  @autoreleasepool
  {
    glfwPollEvents();

    int w, h;
    glfwGetFramebufferSize(glfw_win, &w, &h);

    NSWindow *nswin = glfwGetCocoaWindow(glfw_win);
    CAMetalLayer *layer = (CAMetalLayer*)nswin.contentView.layer;
    layer.drawableSize = CGSizeMake(w, h);
    // or use GetIO().DisplaySize and layer.contentsScale ?
    id <CAMetalDrawable> drawable = [layer nextDrawable];

    render_pass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0); // neededrender_pass
    render_pass.colorAttachments[0].texture = drawable.texture;
    render_pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    render_pass.colorAttachments[0].storeAction = MTLStoreActionStore;

    id <MTLCommandBuffer> cmd_buf = [cmd_q commandBuffer];
    id <MTLRenderCommandEncoder> enc = [cmd_buf renderCommandEncoderWithDescriptor:render_pass];

    ImGui_ImplMetal_NewFrame(render_pass);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin("Clap Example", NULL, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoResize);
    plugin->plugin_impl__draw();
    ImGui::End();

    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd_buf, enc);

    [enc endEncoding];
    [cmd_buf presentDrawable:drawable];
    [cmd_buf commit];
  }
}
@end

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool Plugin::imgui__attach(void *parent)
{
  if (!parent) return false;
  if (m_ui_ctx) return true;

  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) return false;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *glfw_win=glfwCreateWindow(400, 400, "Clap Example", NULL, NULL);
  if (!glfw_win) return false;

  id <MTLDevice> device = MTLCreateSystemDefaultDevice();
  id <MTLCommandQueue> cmd_q = [device newCommandQueue];
  CAMetalLayer *layer = [CAMetalLayer layer];
  layer.device = device;
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  layer.opaque = YES;

  NSWindow *nswin = glfwGetCocoaWindow(glfw_win);
  nswin.contentView.layer = layer;
  nswin.contentView.wantsLayer = YES;
  //[(NSView*)parent addSubview:nswin.contentView];

  // retain this because imgui caches the render pipeline state for each render pass descriptor
  MTLRenderPassDescriptor *render_pass = [MTLRenderPassDescriptor new];

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(glfw_win, true); // needed for metal too
  ImGui_ImplMetal_Init(device);

  ui_ctx_rec *rec = [ui_ctx_rec new];
  rec->plugin=this;
  rec->glfw_win = glfw_win;
  rec->cmd_q = cmd_q;
  rec->render_pass = render_pass;

  rec->timer = [NSTimer scheduledTimerWithTimeInterval:0.030
    target:rec selector:@selector(imgui__on_timer:) userInfo:nil repeats:YES];

  return rec;
}

void Plugin::imgui__destroy()
{
  if (!m_ui_ctx) return;

  ui_ctx_rec *rec=(ui_ctx_rec*)m_ui_ctx;
  m_ui_ctx=NULL;

  [rec->timer invalidate];
  // release rec->blah
  [rec release];

  ImGui_ImplMetal_Shutdown();
}


////////////////////////////////

#if 0 // opengl


#include <OpenGL/gl.h>

@interface ui_ctx_rec : NSObject
{
@public
  Plugin *plugin;
  GLFWwindow *glfw_win;
  NSTimer *timer;
}
-(void)imgui__on_timer:(id)sender;
@end

@implementation ui_ctx_rec
-(void)imgui__on_timer:(id)sender
{
  //glfwPollEvents();

  int w, h;
  glfwGetFramebufferSize(glfw_win, &w, &h);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(w, h));

  ImGui::Begin("Clap Example", NULL, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoResize);
  plugin->plugin_impl__draw();
  ImGui::End();

  ImGui::Render();
  glViewport(0, 0, w, h);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(glfw_win);
}
@end

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool Plugin::imgui__attach(void *parent)
{
  if (!parent) return false;
  if (m_ui_ctx) return true;

  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) return false;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  GLFWwindow *glfw_win=glfwCreateWindow(400, 400, "Clap Example", NULL, NULL);
  if (!glfw_win) return false;

  ImGui_ImplGlfw_InitForOpenGL(glfw_win, true);
  ImGui_ImplOpenGL3_Init(NULL);

/*
  HWND hwnd=glfwGetWin32Window(glfw_win);
  SetParent(hwnd, (HWND)parent);
  long style=GetWindowLong(hwnd, GWL_STYLE);
  style &= ~WS_POPUP;
  style |= WS_CHILDWINDOW;
  SetWindowLong(hwnd, GWL_STYLE, style);
  ShowWindow(hwnd, SW_SHOW);
*/
  ui_ctx_rec *rec = [ui_ctx_rec new];
  rec->plugin=this;
  rec->glfw_win = glfw_win;

  rec->timer = [NSTimer scheduledTimerWithTimeInterval:0.030
    target:rec selector:@selector(imgui__on_timer:) userInfo:nil repeats:YES];

  return true;
}

void Plugin::imgui__destroy()
{
  if (!m_ui_ctx) return;

  ui_ctx_rec *rec=(ui_ctx_rec*)m_ui_ctx;
  m_ui_ctx=NULL;

  [rec->timer invalidate];
  free(rec);

  ImGui_ImplOpenGL3_Shutdown();
}

#endif
