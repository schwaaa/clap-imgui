// passthrough functions.
// add generic passthroughs for extensions here.
// plugin implementations should not need to touch this file.

#include <string.h>
#include "main.h"


Plugin::Plugin(const clap_plugin_descriptor *descriptor, const clap_host* host)
{
  m_w=0;
  m_h=0;
  m_ui_ctx=NULL;

  m_clap_host=*host;

  m_clap_plugin.desc=descriptor;
  m_clap_plugin.plugin_data=this;
  m_clap_plugin.init=plugin::init;
  m_clap_plugin.destroy=plugin::destroy;
  m_clap_plugin.activate=plugin::activate;
  m_clap_plugin.deactivate=plugin::deactivate;
  m_clap_plugin.start_processing=plugin::start_processing;
  m_clap_plugin.stop_processing=plugin::stop_processing;
  m_clap_plugin.process=plugin::process;
  m_clap_plugin.get_extension=plugin::get_extension;
  m_clap_plugin.on_main_thread=plugin::on_main_thread;

  m_clap_plugin_gui.create=gui::create;
  m_clap_plugin_gui.destroy=gui::destroy;
  m_clap_plugin_gui.set_scale=gui::set_scale;
  m_clap_plugin_gui.get_size=gui::get_size;
  m_clap_plugin_gui.round_size=gui::round_size;
  m_clap_plugin_gui.set_size=gui::set_size;
  m_clap_plugin_gui.show=gui::show;
  m_clap_plugin_gui.hide=gui::hide;

  m_clap_plugin_gui_os.attach=gui::attach;

  m_clap_plugin_params.count=params::count;
  m_clap_plugin_params.get_info=params::get_info;
  m_clap_plugin_params.get_value=params::get_value;
  m_clap_plugin_params.value_to_text=params::value_to_text;
  m_clap_plugin_params.text_to_value=params::text_to_value;
  m_clap_plugin_params.flush=params::flush;
}

Plugin::~Plugin()
{
  gui__destroy();
  gui__on_plugin_destroy();
}

bool plugin::init(const clap_plugin *plugin)
{
  return ((Plugin*)plugin->plugin_data)->plugin_impl__init();
}
void plugin::destroy(const clap_plugin *plugin)
{
  delete (Plugin*)plugin->plugin_data;
}
bool plugin::activate(const clap_plugin *plugin, double sample_rate, uint32_t min_frames_count, uint32_t max_frames_count)
{
  return ((Plugin*)plugin->plugin_data)->plugin_impl__activate(sample_rate, min_frames_count, max_frames_count);
}
void plugin::deactivate(const clap_plugin *plugin)
{
  ((Plugin*)plugin->plugin_data)->plugin_impl__deactivate();
}
bool plugin::start_processing(const clap_plugin *plugin)
{
  return ((Plugin*)plugin->plugin_data)->plugin_impl__start_processing();
}
void plugin::stop_processing(const clap_plugin *plugin)
{
  ((Plugin*)plugin->plugin_data)->plugin_impl__stop_processing();
}
clap_process_status plugin::process(const clap_plugin *plugin, const clap_process *process)
{
  return ((Plugin*)plugin->plugin_data)->plugin_impl__process(process);
}
const void* plugin::get_extension(const clap_plugin *plugin, const char* id)
{
  if (!strcmp(id, CLAP_EXT_GUI)) return &((Plugin*)plugin->plugin_data)->m_clap_plugin_gui;
  if (!strcmp(id, CLAP_EXT_GUI_COCOA)) return &((Plugin*)plugin->plugin_data)->m_clap_plugin_gui_os;
  if (!strcmp(id, CLAP_EXT_GUI_WIN32)) return &((Plugin*)plugin->plugin_data)->m_clap_plugin_gui_os;
  if (!strcmp(id, CLAP_EXT_PARAMS)) return &((Plugin*)plugin->plugin_data)->m_clap_plugin_params;
  return ((Plugin*)plugin->plugin_data)->plugin_impl__get_extension(id);
}
void plugin::on_main_thread(const clap_plugin *plugin)
{
  ((Plugin*)plugin->plugin_data)->plugin_impl__on_main_thread();
}

bool gui::create(const clap_plugin *plugin)
{
  return ((Plugin*)plugin->plugin_data)->gui__create();
}
void gui::destroy(const clap_plugin *plugin)
{
  ((Plugin*)plugin->plugin_data)->gui__destroy();
}
void gui::set_scale(const clap_plugin *plugin, double scale)
{
  ((Plugin*)plugin->plugin_data)->gui__set_scale(scale);
}
bool gui::get_size(const clap_plugin *plugin, uint32_t *width, uint32_t *height)
{
  return ((Plugin*)plugin->plugin_data)->gui__get_size(width, height);
}
bool gui::can_resize(const clap_plugin *plugin)
{
  return ((Plugin*)plugin->plugin_data)->gui__can_resize();
}
void gui::round_size(const clap_plugin *plugin, uint32_t *width, uint32_t *height)
{
  ((Plugin*)plugin->plugin_data)->gui__round_size(width, height);
}
bool gui::set_size(const clap_plugin *plugin, uint32_t width, uint32_t height)
{
  return ((Plugin*)plugin->plugin_data)->gui__set_size(width, height);
}
void gui::show(const clap_plugin *plugin)
{
  ((Plugin*)plugin->plugin_data)->gui__show();
}
void gui::hide(const clap_plugin *plugin)
{
  ((Plugin*)plugin->plugin_data)->gui__hide();
}
bool gui::attach(const clap_plugin *plugin, void *window)
{
  return ((Plugin*)plugin->plugin_data)->gui__attach(window);
}

uint32_t params::count(const clap_plugin *plugin)
{
  return ((Plugin*)plugin->plugin_data)->params__count();
}
bool params::get_info(const clap_plugin *plugin, int32_t param_index, clap_param_info_t *param_info)
{
  return ((Plugin*)plugin->plugin_data)->params__get_info(param_index, param_info);
}
bool params::get_value(const clap_plugin *plugin, clap_id param_id, double *value)
{
  return ((Plugin*)plugin->plugin_data)->params__get_value(param_id, value);
}
bool params::value_to_text(const clap_plugin *plugin, clap_id param_id, double value, char *display, uint32_t size)
{
  return ((Plugin*)plugin->plugin_data)->params__value_to_text(param_id, value, display, size);
}
bool params::text_to_value(const clap_plugin *plugin, clap_id param_id, const char *display, double *value)
{
  return ((Plugin*)plugin->plugin_data)->params__text_to_value(param_id, display, value);
}
void params::flush(const clap_plugin *plugin, const clap_input_events *in, const clap_output_events *out)
{
  return ((Plugin*)plugin->plugin_data)->params__flush(in, out);
}
