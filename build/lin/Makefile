TARGET := clap-example.clap
CC := gcc
CXX := g++
FLAGS := -DCLAPPLUGIN_EXPORTS -D_GLFW_X11 -DGLFW_EXPOSE_NATIVE_X11 -fPIC -shared

BASE := ../..

SRC := $(BASE)/src
SRCS := $(SRC)/main.cpp $(SRC)/plugin.cpp \
        $(SRC)/plugin_impl_0.cpp $(SRC)/plugin_impl_1.cpp \
	$(SRC)/gui.cpp $(SRC)/imgui_base.cpp $(SRC)/imgui_lin.cpp
INC := -I$(BASE)/src/imgui -I$(BASE)/src/glfw/include
OBJ := obj

OBJS := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))

$(OBJ)/%.o: $(SRC)/%.cpp
	mkdir -p $(@D)
	$(CXX) -c $(INC) $(FLAGS) $< -o $@

IMGUI_SRC := $(BASE)/src/imgui
IMGUI_SRCS := $(IMGUI_SRC)/imgui.cpp $(IMGUI_SRC)/imgui_draw.cpp \
	$(IMGUI_SRC)/imgui_tables.cpp $(IMGUI_SRC)/imgui_widgets.cpp \
        $(IMGUI_SRC)/imgui_knob.cpp \
	$(IMGUI_SRC)/backends/imgui_impl_glfw.cpp \
	$(IMGUI_SRC)/backends/imgui_impl_opengl3.cpp
IMGUI_OBJ := obj/imgui

INC += $(IMGUI_INC)
OBJS += $(patsubst $(IMGUI_SRC)/%.cpp,$(IMGUI_OBJ)/%.o,$(IMGUI_SRCS))

$(IMGUI_OBJ)/%.o: $(IMGUI_SRC)/%.cpp
	mkdir -p $(@D)
	$(CXX) -c $(INC) $(FLAGS) $< -o $@

GLFW_SRC := $(BASE)/src/glfw/src
GLFW_SRCS := $(GLFW_SRC)/context.c $(GLFW_SRC)/init.c \
	$(GLFW_SRC)/input.c $(GLFW_SRC)/monitor.c \
	$(GLFW_SRC)/platform.c $(GLFW_SRC)/window.c $(GLFW_SRC)/vulkan.c \
        $(GLFW_SRC)/egl_context.c $(GLFW_SRC)/glx_context.c $(GLFW_SRC)/osmesa_context.c \
        $(GLFW_SRC)/posix_module.c $(GLFW_SRC)/posix_thread.c $(GLFW_SRC)/posix_time.c \
        $(GLFW_SRC)/null_init.c $(GLFW_SRC)/null_joystick.c \
        $(GLFW_SRC)/null_monitor.c $(GLFW_SRC)/null_window.c \
        $(GLFW_SRC)/linux_joystick.c $(GLFW_SRC)/xkb_unicode.c \
	$(GLFW_SRC)/x11_init.c $(GLFW_SRC)/x11_monitor.c $(GLFW_SRC)/x11_window.c
GLFW_OBJ := obj/glfw

OBJS += $(patsubst $(GLFW_SRC)/%.c,$(GLFW_OBJ)/%.o,$(GLFW_SRCS))

$(GLFW_OBJ)/%.o: $(GLFW_SRC)/%.c
	mkdir -p $(@D)
	$(CC) -D _GLFW_X11 -c $(FLAGS) $< -o $@

LIBS := -lm -ldl -lpthread 

clap-imgui: $(OBJS)
	$(CXX) -o $(TARGET) $(FLAGS) $(OBJS) $(LIBS)

clean:
	rm $(TARGET) $(OBJS)
