// plugin implementation.
// note this example does very little error checking,
// uses unsafe sprintf/strcpy, etc.

#include <string.h>
#include <math.h>
#include <stdio.h>
#include "main.h"
#include "imgui/imgui.h"


clap_plugin_descriptor example_descriptor =
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
  ""
};

struct Example : public Plugin
{
  int m_srate;
  double m_vol, m_pan;
  double m_peak_in[2], m_peak_out[2];

  Example(const clap_host* host) : Plugin(&example_descriptor, host)
  {
    m_srate=48000;
    m_vol=1.0;
    m_pan=0.0;
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
    m_srate=sample_rate;
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
  
  clap_process_status plugin_impl__process(const clap_process* process)
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

        double adj=m_vol;
        if ((c&1) && m_pan < 0.0) adj *= 1.0+m_pan;
        else if (!(c&1) && m_pan > 0.0) adj *= 1.0-m_pan;

        double peak_in=0.0, peak_out=0.0;
        if (out32)
        {
          if (in32)
          {
            for (int i=0; i < process->frames_count; ++i)
            {
              out32[i]=in32[i]*adj;
              if (in32[i] > peak_in) peak_in=in32[i];
              if (out32[i] > peak_out) peak_out=out32[i];
            }
          }
          else
          {
            memset(out32, 0, process->frames_count*sizeof(*out32));
          }
        }
        else if (out64)
        {
          if (in64)
          {
            for (int i=0; i < process->frames_count; ++i)
            {
              out64[i]=in64[i]*adj;
              if (in64[i] > peak_in) peak_in=in64[i];
              if (out64[i] > peak_out) peak_out=out64[i];
            }
          }
          else
          {
            memset(out64, 0, process->frames_count*sizeof(*out64));
          }
        }
        if (b == 0 && c < 2)
        {
          if (peak_in > m_peak_in[c]) m_peak_in[c]=peak_in;
          if (peak_out > m_peak_out[c]) m_peak_out[c]=peak_out;
        }
      }
    }

    return CLAP_PROCESS_CONTINUE;
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
    if (m_vol > 0.001)
    {
      voldb=log(m_vol)*20.0/log(10.0);
      lbl="%+.1f dB";
    }
    ImGui::SliderFloat("Volume", &voldb, -60.0f, 12.0f, lbl, 1.0f);
    if (voldb > -60.0) m_vol=pow(10.0, voldb/20.0);
    else m_vol=0.0;

    float pan=m_pan*100.0;
    ImGui::SliderFloat("Pan", &pan, -100.0f, 100.0f, "%+.1f%%", 1.0f);
    m_pan=0.01*pan;

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

};

clap_plugin_descriptor *plugin_impl__get_descriptor()
{
  return &example_descriptor;
}

Plugin *plugin_impl__create(const clap_host *host)
{
  return new Example(host);
}

