#include <stdint.h>
#include <cstdlib>
#include <string.h>

#if defined _WIN32
  #include <windows.h>
  HINSTANCE g_hinst;
#endif

#include "main.h"


namespace factory
{
  uint32_t get_plugin_count(const clap_plugin_factory *factory)
  {
    return 1;
  }
  const clap_plugin_descriptor *get_plugin_descriptor(const clap_plugin_factory *factory, uint32_t index)
  {
    if (index == 0) return plugin_impl__get_descriptor_0();
    return NULL;
  }
  const clap_plugin *create_plugin(const clap_plugin_factory *factory, const clap_host *host, const char *plugin_id)
  {
    Plugin *plugin=NULL;
    if (!strcmp(plugin_impl__get_descriptor_0()->id, plugin_id)) plugin=plugin_impl__create_0(host);
    if (plugin) return &plugin->m_clap_plugin;
    return NULL;
  }
};

clap_plugin_factory plugin_factory =
{
  factory::get_plugin_count,
  factory::get_plugin_descriptor,
  factory::create_plugin
};

namespace entry
{
  bool init(const char *plugin_path) { return true; }
  void deinit() { }
  const void *get_factory(const char *factory_id) { return &plugin_factory; }
};


extern "C"
{
  CLAP_EXPORT const clap_plugin_entry clap_entry =
  {
    CLAP_VERSION,
    entry::init,
    entry::deinit,
    entry::get_factory
  };

#if defined _WIN32
  BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
  {
    if (fdwReason == DLL_PROCESS_ATTACH) g_hinst=hDllInst;
    return TRUE;
  }
#endif
};
