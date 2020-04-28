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
  PNGSurface png_image { "/home/keithw/stipple-fullsize.png" };

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
  cairo_set_source_rgba( cairo, 1, 0, 0, 1 );
  cairo_fill( cairo );

  /* finish and copy to YUV raster */
  cairo.flush();

  unsigned int stride = cairo.stride();
  Raster420 yuv_raster { 1920, 1080 };
  for ( unsigned int y = 0; y < 1080; y++ ) {
    for ( unsigned int x = 0; x < 1920; x++ ) {
      const float red = cairo.pixels()[y * stride + 2 + ( x * 4 )] / 255.0;
      const float green = cairo.pixels()[y * stride + 1 + ( x * 4 )] / 255.0;
      const float blue = cairo.pixels()[y * stride + 0 + ( x * 4 )] / 255.0;

      const float Ey = 0.7154  * green + 0.0721 * blue + 0.2125 * red;
      const float Epb = -0.386 * green + 0.5000 * blue - 0.115 * red;
      const float Epr = -0.454 * green - 0.046  * blue + 0.500 * red;

      const uint8_t Y = (219 * Ey) + 16;
      const uint8_t Cb = (224 * Epb) + 128;
      const uint8_t Cr = (224 * Epr) + 128;

      yuv_raster.Y.at( x, y ) = Y;
      if ( (x%2) == 0 and (y%2) == 0 ) {
	yuv_raster.Cb.at( x / 2, y / 2 ) = Cb;
	yuv_raster.Cr.at( x / 2, y / 2 ) = Cr;
      }
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
