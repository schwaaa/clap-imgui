# clap-plugin

Minimal example of prototyping CLAP audio plugins using Dear ImGui as the user interface.

This is intended as an example of presenting a UI for a [CLAP](https://github.com/free-audio/clap) plugin. This is not intended to be a full featured framework; the only CLAP extensions supported are the GUI and Params extensions. The example doesn't support events, parameter automation, etc.

The UI is presented via [ImGui](https://github.com/ocornut/imgui), which is designed as a tool for programmers to quickly protoype controls. ImGui lets the programmer put controls and information on the screen with minimal programming effort:

     ImGui::SliderFloat("Volume", &voldb, -60.0f, 12.0f, "%+.1f dB", 1.0f);

The ImGui backend is implemented via [GLFW](https://github.com/glfw/glfw) and OpenGL, but if you're using this example code you probably don't need to touch any of that.

![](https://github.com/schwaaa/clap-plugin/blob/main/clap_imgui_screencap.gif?raw=true)



