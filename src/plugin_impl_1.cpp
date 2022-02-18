// plugin implementation.
// note this example does very little error checking,
// uses unsafe sprintf/strcpy, etc.

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "imgui/imgui.h"

#define _PI 3.1415926535897932384626433832795

// see imgui_knob.cpp
bool ImGui_Knob(const char* label, float* p_value, float v_min, float v_max, const char *fmt);


static const char *_features[] =
{
  "",
  NULL
};

static clap_plugin_descriptor _descriptor =
{
  CLAP_VERSION,
  "com.cockos.clap-example-1",
  "CLAP Tone Generator",
  "cockos",
  "https://reaper.fm",
  "https://reaper.fm",
  "https://reaper.fm",
  "0.0.1",
  "tone generator",
  _features
};

enum {  PARAM_PITCH, PARAM_DETUNE, PARAM_VOLUME, NUM_PARAMS };
static const clap_param_info _param_info[NUM_PARAMS] =
{
  {
    0, CLAP_PARAM_IS_STEPPED | CLAP_PARAM_REQUIRES_PROCESS, NULL,
    "Pitch", "",
    -1.0, 96.0, -1.0 // -1=off
  },
  {
    1, CLAP_PARAM_REQUIRES_PROCESS, NULL,
    "Detune", "",
    -100.0, 100.0, 0.0
  },
  {
    2, CLAP_PARAM_REQUIRES_PROCESS, NULL,
    "Volume", "",
    -60.0, 0.0, -6.0
  },
};

const char *pitchname[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

namespace ports
{
   uint32_t count(const clap_plugin *plugin, bool is_input);
   bool get(const clap_plugin *plugin, uint32_t index, bool is_input, clap_audio_port_info *info);
};

struct Example_1 : public Plugin
{
  int m_srate;
  double m_param_values[NUM_PARAMS];
  double m_last_param_values[NUM_PARAMS];
  double m_phase, m_last_oct;
  int m_hold;

  clap_plugin_audio_ports m_clap_plugin_audio_ports;

  Example_1(const clap_host* host) : Plugin(&_descriptor, host)
  {
    m_srate=48000;
    for (int i=0; i < NUM_PARAMS; ++i)
    {
      m_param_values[i] = m_last_param_values[i] =
        _params__convert_value(i, _param_info[i].default_value);
    }

    m_phase=0.0;
    m_last_oct=4.0;
    m_hold=0;

    m_clap_plugin_audio_ports.count=ports::count;
    m_clap_plugin_audio_ports.get=ports::get;
  }

  ~Example_1()
  {
  }

  uint32_t ports__count(bool is_input)
  {
    return is_input ? 0 : 1;
  }

  bool ports__get(uint32_t index, bool is_input, clap_audio_port_info *info)
  {
    if (!is_input && index == 0)
    {
      if (info)
      {
        memset(info, 0, sizeof(clap_audio_port_info));
        info->id = 1;
        strcpy(info->name, "Stereo Out");
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = CLAP_INVALID_ID;
      }
      return true;
    }
    return false;
  }

  bool plugin_impl__init()
  {
    return true;
  }

  bool plugin_impl__activate(double sample_rate, uint32_t min_frames_count, uint32_t max_frames_count)
  {
    m_srate=(int)sample_rate;
    m_phase=0.0;
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
  clap_process_status _plugin_impl__process(const clap_process *process,
    int num_channels, int start_frame, int end_frame,
    double *start_param_values, double *end_param_values,
    T **out)
  {
    if (!out) return CLAP_PROCESS_ERROR;

    double start_vol = start_param_values[PARAM_VOLUME];
    double end_vol = end_param_values[PARAM_VOLUME];
    double d_vol = (end_vol-start_vol) / (double)(end_frame-start_frame);

    double start_pitch = start_param_values[PARAM_PITCH];
    double end_pitch = end_param_values[PARAM_PITCH];
    double start_phase=m_phase, d_phase=0.0;
    if (end_pitch >= 0.0)
    {
      if (start_pitch < 0.0) start_phase=0.0;
      else start_pitch += start_param_values[PARAM_DETUNE]*0.01;
      end_pitch += end_param_values[PARAM_DETUNE]*0.01;
      double freq = 440.0 * pow(2.0, (end_pitch-57.0)/12.0);
      d_phase = 2.0 * _PI * freq / (double)m_srate;
    }

    for (int c=0; c < num_channels; ++c)
    {
      T *cout=out[c];
      if (!cout) return CLAP_PROCESS_ERROR;

      if (d_phase > 0.0)
      {
        double vol = start_vol;
        double phase = start_phase;
        for (int i=start_frame; i < end_frame; ++i)
        {
          cout[i] = sin(phase)*vol;
          phase += d_phase;
          vol += d_vol;
        }
      }
      else
      {
        memset(cout+start_frame, 0, (end_frame-start_frame)*sizeof(T));
      }
    }

    m_phase += (double)(end_frame-start_frame)*d_phase;

    return CLAP_PROCESS_CONTINUE;
  }

  clap_process_status plugin_impl__process(const clap_process *process)
  {
    double cur_param_values[NUM_PARAMS];
    for (int i=0; i < NUM_PARAMS; ++i)
    {
      cur_param_values[i]=m_param_values[i];
    }

    clap_process_status s = -1;
    if (process && process->audio_inputs_count == 0 &&
      process->audio_outputs_count == 1 && process->audio_outputs[0].channel_count == 2)
    {
      // handling incoming parameter changes and slicing the process call
      // on the time axis would happen here.

      if (process->audio_outputs[0].data32)
      {
        s = _plugin_impl__process(process, 2, 0, process->frames_count,
          m_last_param_values, cur_param_values,
          process->audio_outputs[0].data32);
      }
      else if (process->audio_outputs[0].data64)
      {
        s = _plugin_impl__process(process, 2, 0, process->frames_count,
          m_last_param_values, cur_param_values,
          process->audio_outputs[0].data64);
      }
    }

    for (int i=0; i < NUM_PARAMS; ++i)
    {
      m_last_param_values[i]=m_param_values[i];
    }

    if (s < 0) s = CLAP_PROCESS_ERROR;
    return s;
  }

  const void* plugin_impl__get_extension(const char* id)
  {
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &m_clap_plugin_audio_ports;
    return NULL;
  }

  void plugin_impl__on_main_thread()
  {
  }

  void plugin_impl__draw()
  {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0, 16.0));

    ImGui::Text("Tone Generator");

    int cur_pitch = (int)m_param_values[PARAM_PITCH];
    int pitch_button=-1;
    for (int i=0; i < 12; ++i)
    {
      if (i) ImGui::SameLine();
      double h = (double)i/12.0;
      ImColor up = ImColor::HSV(h, 0.5, 0.375), hover = ImColor::HSV(h, 0.5, 0.625), down = ImColor::HSV(h, 0.5, 1.0);
      if (i == cur_pitch%12) up=hover=down;
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(up));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(hover));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(down));
      char buf[64];
      sprintf(buf, "%-2s", pitchname[i]);
      ImGui::Button(buf);
      ImGui::PopStyleColor(3);
      if (ImGui::IsItemActive()) pitch_button=i;
    }

    ImGui::SameLine(0.0, 24.0);
    int held = m_hold;
    ImColor col(0.0f, 0.5f, 1.0f, 1.0f);
    if (held) ImGui::PushStyleColor(ImGuiCol_Button, col.Value);
    if (ImGui::Button(" hold ")) m_hold = !m_hold;
    if (held) ImGui::PopStyleColor();

    if (pitch_button >= 0)
    {
      m_param_values[PARAM_PITCH] = (double)(pitch_button + (int)(m_last_oct+0.5)*12);
    }
    else if (m_hold && cur_pitch >= 0)
    {
      m_param_values[PARAM_PITCH] = (double)(cur_pitch%12 + (int)(m_last_oct+0.5)*12);
    }
    else
    {
      m_param_values[PARAM_PITCH] = -1;
    }

    ImGui::Spacing();

    float oct = m_last_oct;
    ImGui_Knob("Octave", &oct, 1.0f, 8.0f, "%.0f");
    m_last_oct = oct;

    ImGui::SameLine(0.0, 24.0);

    float detune=(float)m_param_values[PARAM_DETUNE];
    ImGui_Knob("Detune", &detune, -100.0f, 100.0f, "%+.0f%%");
    m_param_values[PARAM_DETUNE]=detune;

    ImGui::SameLine(0.0, 24.0);

    float voldb=-60.0;
    const char *lbl="-inf";
    if (m_param_values[PARAM_VOLUME] > 0.001)
    {
      voldb=log(m_param_values[PARAM_VOLUME])*20.0/log(10.0);
      lbl="%+.1f dB";
    }
    ImGui_Knob("Volume", &voldb, -60.0, 0.0, lbl);
    if (voldb > -60.0) m_param_values[PARAM_VOLUME]=pow(10.0, voldb/20.0);
    else m_param_values[PARAM_VOLUME]=0.0;

    ImGui::PopStyleVar();
  }

  bool plugin_impl__get_preferred_size(uint32_t* width, uint32_t* height)
  {
    *width=600;
    *height=300;
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

    if (param_id == PARAM_PITCH)
    {
      *value = m_param_values[PARAM_PITCH];
    }
    else if (param_id == PARAM_DETUNE)
    {
      *value = m_param_values[PARAM_DETUNE];
    }
    else if (param_id == PARAM_VOLUME)
    {
      if (m_param_values[PARAM_VOLUME] <= 0.0) *value = -150.0;
      else *value = log(m_param_values[PARAM_VOLUME])*20.0/log(10.0);
    }
    return true;
  }

  static double _params__convert_value(clap_id param_id, double in_value)
  {
    // convert from external value to internal value.

    if (param_id < 0 || param_id >= NUM_PARAMS) return 0.0;

    if (param_id == PARAM_PITCH)
    {
      return in_value;
    }
    else if (param_id == PARAM_DETUNE)
    {
      return in_value;
    }
    if (param_id == PARAM_VOLUME)
    {
      return in_value > -150.0 ? pow(10.0, in_value/20.0) : 0.0;
    }

    return 0.0;
  }

  bool params__value_to_text(clap_id param_id, double value, char *display, uint32_t size)
  {
    if (!display || !size) return false;
    if (param_id < 0 || param_id >= NUM_PARAMS) return false;

    if (param_id == PARAM_PITCH)
    {
      int pitch=(int)value;
      if (pitch < 0 || pitch > 144) strcpy(display, "off");
      else sprintf(display, "%s%d", pitchname[pitch%12], pitch/12+1);
    }
    else if (param_id == PARAM_DETUNE)
    {
      sprintf(display, "%+.0f", value);
    }
    else if (param_id == PARAM_VOLUME)
    {
      if (value <= -150.0) strcpy(display, "-inf");
      else sprintf(display, "%+.2f", value);
    }
    return true;
  }

  bool params__text_to_value(clap_id param_id, const char *display, double *value)
  {
    if (!display || !value) return false;
    if (param_id < 0 || param_id >= NUM_PARAMS) return false;

    if (param_id == PARAM_PITCH)
    {
      *value=-1.0;
      for (int i=0; i < 12; ++i)
      {
        if (!strncmp(pitchname[i], display, strlen(pitchname[i])))
        {
          *value = (int)(i + atoi(display+strlen(pitchname[i]))*12);
          break;
        }
      }
    }
    else if (param_id == PARAM_DETUNE)
    {
      *value=atof(display);
    }
    else if (param_id == PARAM_VOLUME)
    {
      if (!strcmp(display, "-inf")) *value=-150.0;
      else *value=atof(display);
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

uint32_t ports::count(const clap_plugin *plugin, bool is_input)
{
  return ((Example_1*)plugin->plugin_data)->ports__count(is_input);
}
bool ports::get(const clap_plugin *plugin, uint32_t index, bool is_input, clap_audio_port_info *info)
{
  return ((Example_1*)plugin->plugin_data)->ports__get(index, is_input, info);
}

clap_plugin_descriptor *plugin_impl__get_descriptor_1()
{
  return &_descriptor;
}

Plugin *plugin_impl__create_1(const clap_host *host)
{
  return new Example_1(host);
}

