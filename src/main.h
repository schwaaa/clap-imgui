// add generic support for extensions here.
// plugin implementations should not need to touch this file.

#pragma once

#define _ALLOW_KEYWORD_MACROS
#define alignas(x)
#include "clap/include/clap/clap.h"

struct Plugin
{
  clap_plugin m_clap_plugin;
  clap_plugin_params m_clap_plugin_params;
  int m_w, m_h;

  clap_plugin_gui m_clap_plugin_gui;
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

  bool gui__is_api_supported(const char *api, bool is_floating);
  bool gui__create(const char *api, bool is_floating);
  bool gui__destroy(bool is_plugin_destroy);
  bool gui__set_scale(double scale);
  bool gui__get_size(uint32_t *width, uint32_t *height);
  bool gui__can_resize();
  bool gui__adjust_size(uint32_t *width, uint32_t *height);
  bool gui__set_size(uint32_t width, uint32_t height);
  bool gui__set_parent(const clap_window *window);
  bool gui__set_transient(const clap_window *window);
  void gui__suggest_title(const char *title);
  bool gui__show();
  bool gui__hide();

  virtual uint32_t params__count()=0;
  virtual bool params__get_info(uint32_t param_index, clap_param_info_t *param_info)=0;
  virtual bool params__get_value(clap_id param_id, double *value)=0;
  virtual bool params__value_to_text(clap_id param_id, double value, char *display, uint32_t size)=0;
  virtual bool params__text_to_value(clap_id param_id, const char *display, double *value)=0;
  virtual void params__flush(const clap_input_events *in, const clap_output_events *out)=0;
};

clap_plugin_descriptor *plugin_impl__get_descriptor_0();
Plugin *plugin_impl__create_0(const clap_host *host);

clap_plugin_descriptor *plugin_impl__get_descriptor_1();
Plugin *plugin_impl__create_1(const clap_host *host);


