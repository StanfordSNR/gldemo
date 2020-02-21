AM_CPPFLAGS = $(CXX17_FLAGS) $(GLU_CFLAGS) $(GLEW_CFLAGS) $(GLFW3_CFLAGS) $(PANGOCAIRO_CFLAGS) -I$(srcdir)/../util
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = example drawtext

example_SOURCES = example.cc
example_LDADD = ../util/libgldemoutil.a $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS) $(PANGOCAIRO_LIBS)

drawtext_SOURCES = drawtext.cc
drawtext_LDADD = ../util/libgldemoutil.a $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS) $(PANGOCAIRO_LIBS)
