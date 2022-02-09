// plugin implementation.
// note this example does very little error checking,
// uses unsafe sprintf/strcpy, etc.

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "imgui/imgui.h"


const char *_features[] = 
{
  "",
  NULL
};

clap_plugin_descriptor _descriptor =
{
  CLAP_VERSION,
  "com.cockos.clap-example",
  "clap-example",
  "cockos",
  "https://reaper.fm",
  "https://reaper.fm",
  "https://reaper.fm",
  "0.0.1",
  "example",
  _features
};

enum { PARAM_VOLUME, PARAM_PAN, NUM_PARAMS };
static const clap_param_info _param_info[NUM_PARAMS] =
{
  {
    0, CLAP_PARAM_REQUIRES_PROCESS, NULL,
    "Volume", "",
    -60.0, 12.0, 0.0
  },
  {
    1, CLAP_PARAM_REQUIRES_PROCESS, NULL,
    "Pan", "",
    -100.0, 100.0, 0.0
  }
};

struct Example : public Plugin
{
  int m_srate;
  double m_param_values[NUM_PARAMS];
  double m_last_param_values[NUM_PARAMS];
  double m_peak_in[2], m_peak_out[2];

  Example(const clap_host* host) : Plugin(&_descriptor, host)
  {
    m_srate=48000;
    for (int i=0; i < NUM_PARAMS; ++i)
    {
      m_param_values[i] = m_last_param_values[i] =
        _params__convert_value(i, _param_info[i].default_value);
    }
    m_peak_in[0]=m_peak_in[1]=m_peak_out[0]=m_peak_out[1]=0.0;
  }

  ~Example()
  {
  }

  bool plugin_impl__init()
  {
    return true;
  }

  bool plugin_impl__activate(double sample_rate, uint32_t min_frames_count, uint32_t max_frames_count)
  {
    m_srate=(int)sample_rate;
    m_peak_in[0]=m_peak_in[1]=m_peak_out[0]=m_peak_out[1]=0.0;
    return true;   
  }
 
  void plugin_impl__deactivate()
  {
  }

  bool plugin_impl__start_processing()
  {
    return true;
  }
 
  void plugin_impl__stop_processing()
  {
  }

  template <class T>
  void _plugin_impl__process_channel(const clap_process *process,
    int start_frame, int end_frame,
    double *start_param_values, double *end_param_values,
    int bus, int channel, T *in, T *out)
  {
    // many plugin implementations may not be templatizable per-channel this way.

    double adj=start_param_values[PARAM_VOLUME];
    if ((channel&1) && start_param_values[PARAM_PAN] < 0.0)
    {
      adj *= 1.0+start_param_values[PARAM_PAN];
    }
    else if (!(channel&1) && start_param_values[PARAM_PAN] > 0.0)
    {
      adj *= 1.0-start_param_values[PARAM_PAN];
    }

    double end_adj=end_param_values[PARAM_VOLUME];
    if ((channel&1) && end_param_values[PARAM_PAN] < 0.0)
    {
      end_adj *= 1.0+end_param_values[PARAM_PAN];
    }
    else if (!(channel&1) && end_param_values[PARAM_PAN] > 0.0)
    {
      end_adj *= 1.0-end_param_values[PARAM_PAN];
    }

    double d_adj=0.0;
    if (end_frame > start_frame)
    {
      d_adj = (end_adj-adj)/(double)(end_frame-start_frame);
    }

    double peak_in=0.0, peak_out=0.0;
    if (in)
    {
      for (int i=start_frame; i < end_frame; ++i)
      {
        out[i]=in[i]*adj;
        if (in[i] > peak_in) peak_in=in[i];
        if (out[i] > peak_out) peak_out=out[i];
        adj += d_adj;
      }
    }
    else
    {
      memset(out, 0, process->frames_count*sizeof(T));
    }

    if (bus == 0 && channel < 2)
    {
      if (peak_in > m_peak_in[channel]) m_peak_in[channel]=peak_in;
      if (peak_out > m_peak_out[channel]) m_peak_out[channel]=peak_out;
    }
  }

  clap_process_status _plugin_impl__process(const clap_process *process,
    int start_frame, int end_frame,
    double *start_param_values, double *end_param_values)
  {
    for (int b=0; b < process->audio_outputs_count; ++b)
    {
      const clap_audio_buffer *outbuf=process->audio_outputs+b;
      for (int c=0; c < outbuf->channel_count; ++c)
      {
        float *out32=NULL,*in32=NULL;
        double *out64=NULL, *in64=NULL;

        if (outbuf->data32) out32=outbuf->data32[c];
        else if (outbuf->data64) out64=outbuf->data64[c];

        if (b < process->audio_inputs_count)
        {
          const clap_audio_buffer *inbuf=process->audio_inputs+b;
          if (c < inbuf->channel_count)
          {
            if (inbuf->data32) in32=inbuf->data32[c];
            else if (inbuf->data64) in64=inbuf->data64[c];
          }
        }

        if (out32)
        {
          _plugin_impl__process_channel(process,
            start_frame, end_frame, start_param_values, end_param_values,
            b, c, in32, out32);
        }
        else if (out64)
        {
          _plugin_impl__process_channel(process,
            start_frame, end_frame, start_param_values, end_param_values,
            b, c, in64, out64);
        }
      }
    }

    return CLAP_PROCESS_CONTINUE;
  }

  clap_process_status plugin_impl__process(const clap_process *process)
  {
    if (!process) return CLAP_PROCESS_ERROR;

    const double decay=pow(0.5, (double)process->frames_count/(double)m_srate/0.125);
    for (int c=0; c < 2; ++c)
    {
      m_peak_in[c] *= decay;
      m_peak_out[c] *= decay;
      if (m_peak_in[c] < 1.0e-6) m_peak_in[c]=0.0;
      if (m_peak_out[c] < 1.0e-6) m_peak_out[c]=0.0;
    }

    // handling incoming parameter changes and slicing the process call
    // on the time axis would happen here.

    clap_process_status s=_plugin_impl__process(process,
      0, process->frames_count, m_last_param_values, m_param_values);

    for (int i=0; i < NUM_PARAMS; ++i)
    {
      m_last_param_values[i]=m_param_values[i];
    }

    return s;
  }

  const void* plugin_impl__get_extension(const char* id)
  {
   return NULL;
  }

  void plugin_impl__on_main_thread()
  {
  }

  void plugin_impl__draw()
  {
    ImGui::Text("Clap Example Plugin");

    for (int c=0; c < 2; ++c)
    {
      double val=0.0;
      if (m_peak_in[c] > 0.001)
      {
        double db=log(m_peak_in[c])*20.0/log(10.0);
        val=(db+60.0)/72.0;
      }
      ImGui::ProgressBar(val, ImVec2(-FLT_MIN, 0), "");
    }

    float voldb=-60.0;
    const char *lbl="-inf";
    if (m_param_values[PARAM_VOLUME] > 0.001)
    {
      voldb=log(m_param_values[PARAM_VOLUME])*20.0/log(10.0);
      lbl="%+.1f dB";
    }
    ImGui::SliderFloat("Volume", &voldb, -60.0f, 12.0f, lbl, 1.0f);
    if (voldb > -60.0) m_param_values[PARAM_VOLUME]=pow(10.0, voldb/20.0);
    else m_param_values[PARAM_VOLUME]=0.0;

    float pan=m_param_values[PARAM_PAN]*100.0;
    ImGui::SliderFloat("Pan", &pan, -100.0f, 100.0f, "%+.1f%%", 1.0f);
    m_param_values[PARAM_PAN]=0.01*pan;

    for (int c=0; c < 2; ++c)
    {
      double val=0.0;
      if (m_peak_out[c] > 0.001)
      {
        double db=log(m_peak_out[c])*20.0/log(10.0);
        val=(db+60.0)/72.0;
      }
      ImGui::ProgressBar(val, ImVec2(-FLT_MIN, 0), "");
    }
  }

  bool plugin_impl__get_preferred_size(uint32_t* width, uint32_t* height)
  {
    *width=400;
    *height=400;
    return true;
  }

  uint32_t params__count()
  {
    return NUM_PARAMS;
  }

  bool params__get_info(uint32_t param_index, clap_param_info_t *param_info)
  {
    if (param_index < 0 || param_index >= NUM_PARAMS) return false;
    *param_info=_param_info[param_index];
    return true;
  }

  bool params__get_value(clap_id param_id, double *value)
  {
    if (!value) return false;
    if (param_id < 0 || param_id >= NUM_PARAMS) return false;

    if (param_id == PARAM_VOLUME)
    {
      if (m_param_values[PARAM_VOLUME] <= 0.0) *value = -150.0;
      else *value = log(m_param_values[PARAM_VOLUME])*20.0/log(10.0);
    }
    else if (param_id == PARAM_PAN)
    {
      *value = 100.0*m_param_values[PARAM_PAN];
    }
    return true;
  }

  static double _params__convert_value(clap_id param_id, double in_value)
  {
    // convert from external value to internal value.

    if (param_id < 0 || param_id >= NUM_PARAMS) return 0.0;

    if (param_id == PARAM_VOLUME)
    {
      return in_value > -150.0 ? pow(10.0, in_value/20.0) : 0.0;
    }
    if (param_id == PARAM_PAN)
    {
      return 0.01*in_value;
    }

    return 0.0;
  }

  bool params__value_to_text(clap_id param_id, double value, char *display, uint32_t size)
  {
    if (!display || !size) return false;
    if (param_id < 0 || param_id >= NUM_PARAMS) return false;

    if (param_id == PARAM_VOLUME)
    {
      if (value <= -150.0) strcpy(display, "-inf");
      else sprintf(display, "%+.2f", value);
    }
    else if (param_id == PARAM_PAN)
    {
      sprintf(display, "%+.0f%%", value);
    }
    return true;
  }

  bool params__text_to_value(clap_id param_id, const char *display, double *value)
  {
    if (!display || !value) return false;
    if (param_id < 0 || param_id >= NUM_PARAMS) return false;

    if (param_id == PARAM_VOLUME)
    {
      if (!strcmp(display, "-inf")) *value=-150.0;
      else *value=atof(display);
    }
    else if (param_id == PARAM_PAN)
    {
      *value=atof(display);
    }
    return true;
  }

  void params__flush(const clap_input_events *in, const clap_output_events *out)
  {
    if (!in) return;

    for (int i=0; i < in->size(in); ++i)
    {
      const clap_event_header *evt=in->get(in, i);
      if (!evt || evt->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
      if (evt->type == CLAP_EVENT_PARAM_VALUE)
      {
        const clap_event_param_value *pevt=(const clap_event_param_value*)evt;
        if (pevt->param_id < 0 || pevt->param_id >= NUM_PARAMS) continue; // assert

        m_param_values[pevt->param_id] =
          _params__convert_value(pevt->param_id, pevt->value);
      }
    }
  }

};

clap_plugin_descriptor *plugin_impl__get_descriptor()
{
  return &_descriptor;
}

Plugin *plugin_impl__create(const clap_host *host)
{
  return new Example(host);
}

