AM_CPPFLAGS = $(CXX17_FLAGS) $(GLU_CFLAGS) $(GLEW_CFLAGS) $(GLFW3_CFLAGS) $(PANGOCAIRO_CFLAGS) $(SDL_CFLAGS) $(SDL_GFX_CFLAGS) $(SDL_IMAGE_CFLAGS) $(SDL_TTF_CFLAGS) $(SDL_MIXER_CFLAGS) -I$(srcdir)/../util
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = example drawtext

example_SOURCES = example.cc
example_LDADD = -L/usr/lib -leyelink_core_graphics -leyelink_core -lsdl_util -lrt -leyelink_core_graphics -lpthread ../util/libgldemoutil.a $(SDL_LIBS) $(SDL_GFX_LIBS) $(SDL_IMAGE_LIBS) $(SDL_TTF_LIBS) $(SDL_MIXER_LIBS) $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS) $(PANGOCAIRO_LIBS)

drawtext_SOURCES = drawtext.cc
drawtext_LDADD = ../util/libgldemoutil.a $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS) $(PANGOCAIRO_LIBS)
