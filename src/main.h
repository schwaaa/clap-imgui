// add generic support for extensions here.
// plugin implementations should not need to touch this file.

#pragma once

#define _ALLOW_KEYWORD_MACROS
#define alignas(x)
#include "clap/include/clap/entry.h"
#include "clap/include/clap/plugin-factory.h"
#include "clap/include/clap/ext/gui.h"
#include "clap/include/clap/ext/gui-win32.h"
#include "clap/include/clap/ext/gui-cocoa.h"
#include "clap/include/clap/ext/params.h"


struct clap_plugin_gui_os
{
  bool (*attach)(const clap_plugin *plugin, void *window);
};

struct Plugin
{
  clap_host m_clap_host;
  clap_plugin m_clap_plugin;
  clap_plugin_gui m_clap_plugin_gui;
  clap_plugin_gui_os m_clap_plugin_gui_os;
  clap_plugin_params m_clap_plugin_params;
  int m_w, m_h;

  void *m_ui_ctx;

  Plugin(const clap_plugin_descriptor *descriptor, const clap_host *host);
  virtual ~Plugin();

  virtual bool plugin_impl__init()=0;
  virtual bool plugin_impl__activate(double sample_rate, uint32_t min_frames_count, uint32_t max_frames_count)=0;
  virtual void plugin_impl__deactivate()=0;
  virtual bool plugin_impl__start_processing()=0;
  virtual void plugin_impl__stop_processing()=0;
  virtual clap_process_status plugin_impl__process(const clap_process* process)=0;
  virtual const void* plugin_impl__get_extension(const char* id)=0;
  virtual void plugin_impl__on_main_thread()=0;
  virtual void plugin_impl__draw()=0;
  virtual bool plugin_impl__get_preferred_size(uint32_t *width, uint32_t *height)=0;

  bool gui__create();
  void gui__destroy();
  bool gui__set_scale(double scale);
  bool gui__get_size(uint32_t *width, uint32_t *height);
  bool gui__can_resize();
  void gui__round_size(uint32_t *width, uint32_t *height);
  bool gui__set_size(uint32_t width, uint32_t height);
  void gui__show();
  void gui__hide();
  bool gui__attach(void *window);

  virtual uint32_t params__count()=0;
  virtual bool params__get_info(uint32_t param_index, clap_param_info_t *param_info)=0;
  virtual bool params__get_value(clap_id param_id, double *value)=0;
  virtual bool params__value_to_text(clap_id param_id, double value, char *display, uint32_t size)=0;
  virtual bool params__text_to_value(clap_id param_id, const char *display, double *value)=0;
  virtual void params__flush(const clap_input_events *in, const clap_output_events *out)=0;
};

namespace plugin
{
  bool init(const clap_plugin *plugin);
  void destroy(const clap_plugin *plugin);
  bool activate(const clap_plugin *plugin, double sample_rate, uint32_t min_frames_count, uint32_t max_frames_count);
  void deactivate(const clap_plugin *plugin);
  bool start_processing(const clap_plugin *plugin);
  void stop_processing(const clap_plugin *plugin);
  clap_process_status process(const clap_plugin *plugin, const clap_process *process);
  const void* get_extension(const clap_plugin *plugin, const char *id);
  void on_main_thread(const clap_plugin *plugin);
};

namespace gui
{
  bool create(const clap_plugin *plugin);
  void destroy(const clap_plugin *plugin);
  bool set_scale(const clap_plugin *plugin, double scale);
  bool get_size(const clap_plugin *plugin, uint32_t *width, uint32_t *height);
  bool can_resize(const clap_plugin *plugin);
  void round_size(const clap_plugin *plugin, uint32_t *width, uint32_t *height);
  bool set_size(const clap_plugin *plugin, uint32_t width, uint32_t height);
  void show(const clap_plugin *plugin);
  void hide(const clap_plugin *plugin);
  bool attach(const clap_plugin *plugin, void *window);
};

namespace params
{
  uint32_t count(const clap_plugin *plugin);
  bool get_info(const clap_plugin *plugin, uint32_t param_index, clap_param_info_t *param_info);
  bool get_value(const clap_plugin *plugin, clap_id param_id, double *value);
  bool value_to_text(const clap_plugin *plugin, clap_id param_id, double value, char *display, uint32_t size);
  bool text_to_value(const clap_plugin *plugin, clap_id param_id, const char *display, double *value);
  void flush(const clap_plugin *plugin, const clap_input_events *in, const clap_output_events *out);
};

clap_plugin_descriptor *plugin_impl__get_descriptor();
Plugin *plugin_impl__create(const clap_host *host);
void plugin__initialize(Plugin* plugin);
void gui__on_plugin_destroy();

