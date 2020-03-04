#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>
#include <thread>

#include <unistd.h>

#include "cairo_objects.hh"
#include "display.hh"

using namespace std;
using namespace std::chrono;

void program_body()
{
  VideoDisplay display { 1920, 1080, false }; // fullscreen window @ 1920x1080 luma resolution

  Cairo cairo { 1920, 1080 };
  Pango pango { cairo };

  /* open the PNG */
  PNGSurface png_image { "/home/brookek/repos/gldemo/src/files/tree.png" };

  /* draw gray over everything */
  cairo_new_path( cairo );
  cairo_identity_matrix( cairo );
  cairo_rectangle( cairo, 500, 500, 100, 100 );
  cairo_set_source_rgba( cairo, 0, 0.9, 0, 0.5 );
  cairo_fill( cairo );

  /* draw the PNG */
  cairo_identity_matrix( cairo );
  cairo_scale( cairo, 0.1, 0.1 );
  double center_x = 960, center_y = 540;
  cairo_device_to_user( cairo, &center_x, &center_y );
  cairo_translate( cairo, center_x, center_y );
  cairo_set_source_surface( cairo, png_image, 0, 0 );
  cairo_paint( cairo );

  /* draw some text */
  Pango::Font myfont { "Times New Roman, 80" };
  Pango::Text mystring { cairo, pango, myfont, "Hello, world, Brooke, and Luke." };
  mystring.draw_centered_at( cairo, 960, 540 );
  cairo_set_source_rgba( cairo, 1, 1, 1, 1 );
  cairo_fill( cairo );

  /* finish and copy to YUV raster */
  cairo.flush();

  unsigned int stride = cairo.stride();
  Raster420 yuv_raster { 1920, 1080 };
  for ( unsigned int y = 0; y < 1080; y++ ) {
    for ( unsigned int x = 0; x < 1920; x++ ) {
      yuv_raster.Y.at( x, y ) = cairo.pixels()[y * stride + 1 + ( x * 4 )];
    }
  }

  Texture420 texture { yuv_raster };
  display.draw( texture );
  pause();
}

int main()
{
  try {
    program_body();
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
