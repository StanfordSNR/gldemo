#ifndef CAIRO_OBJECTS_HH
#define CAIRO_OBJECTS_HH

#include <cairo.h>
#include <limits>
#include <memory>
#include <pango/pangocairo.h>

#include "gl_objects.hh"

class Surface
{
  struct Deleter
  {
    void operator()( cairo_surface_t* x ) const;
  };

  std::unique_ptr<cairo_surface_t, Deleter> surface_;

protected:
  Surface( cairo_surface_t* surface_ptr );

public:
  void check_error();

  operator cairo_surface_t*() { return surface_.get(); }
  operator const cairo_surface_t*() const { return surface_.get(); }
};

class ImageSurface : public Surface
{
  unsigned int width_, height_, stride_;

protected:
  ImageSurface( cairo_surface_t* surface_ptr );

public:
  uint8_t* pixels() { return cairo_image_surface_get_data( *this ); }

  unsigned int width() const { return width_; }
  unsigned int height() const { return height_; }
  unsigned int stride() const { return stride_; }
};

class FreshImageSurface : public ImageSurface
{
public:
  FreshImageSurface( const unsigned int width, const unsigned int height )
    : ImageSurface( cairo_image_surface_create( CAIRO_FORMAT_RGB24, width, height ) )
  {}
};

class PNGSurface : public ImageSurface
{
public:
  PNGSurface( const char* filename )
    : ImageSurface( cairo_image_surface_create_from_png( filename ) )
  {}
};

class Cairo
{
  FreshImageSurface surface_;

  struct Context
  {
    struct Deleter
    {
      void operator()( cairo_t* x ) const;
    };

    std::unique_ptr<cairo_t, Deleter> context;

    Context( ImageSurface& surface );

    void check_error();
  } context_;

  void check_error();

public:
  Cairo( const unsigned int width, const unsigned int height );

  unsigned int width() { return surface_.width(); }
  unsigned int height() { return surface_.height(); }
  unsigned int stride() { return surface_.stride(); }

  operator cairo_t*() { return context_.context.get(); }

  uint8_t* pixels() { return surface_.pixels(); }
  void flush() { cairo_surface_flush( surface_ ); }

  template<bool device_coordinates>
  struct Extent
  {
    double x, y, width, height;

    Extent<false> to_user( Cairo& cairo ) const
    {
      static_assert( device_coordinates == true, "Extent::to_user() called but coordinates already in user-space" );

      double x1 = x, x2 = x + width, y1 = y, y2 = y + height;

      cairo_device_to_user( cairo, &x1, &y1 );
      cairo_device_to_user( cairo, &x2, &y2 );

      return Extent<false>( { x1, y1, x2 - x1, y2 - y1 } );
    }

    Extent<true> to_device( Cairo& cairo ) const
    {
      static_assert( device_coordinates == false,
                     "Extent::to_device() called but coordinates already in device-space" );

      double x1 = x, x2 = x + width, y1 = y, y2 = y + height;

      cairo_user_to_device( cairo, &x1, &y1 );
      cairo_user_to_device( cairo, &x2, &y2 );

      return Extent<true>( { x1, y1, x2 - x1, y2 - y1 } );
    }
  };

  class Pattern
  {
    struct Deleter
    {
      void operator()( cairo_pattern_t* x ) const;
    };

    std::unique_ptr<cairo_pattern_t, Deleter> pattern_;

  public:
    Pattern( cairo_pattern_t* pattern );

    operator cairo_pattern_t*() { return pattern_.get(); }
  };
};

template<class T>
struct PangoDelete
{
  void operator()( T* x ) const { g_object_unref( x ); }
};

class Pango
{
  std::unique_ptr<PangoContext, PangoDelete<PangoContext>> context_;
  std::unique_ptr<PangoLayout, PangoDelete<PangoLayout>> layout_;

public:
  Pango( Cairo& cairo );

  operator PangoContext*() { return context_.get(); }
  operator PangoLayout*() { return layout_.get(); }

  struct Font
  {
    struct Deleter
    {
      void operator()( PangoFontDescription* x ) const;
    };

    std::unique_ptr<PangoFontDescription, Deleter> font;

    Font( const std::string& description );

    operator const PangoFontDescription*() const { return font.get(); }
  };

  class Text
  {
    struct Deleter
    {
      void operator()( cairo_path_t* x ) const;
    };

    std::unique_ptr<cairo_path_t, Deleter> path_;
    Cairo::Extent<false> extent_;

  public:
    Text( Cairo& cairo, Pango& pango, const Font& font, const std::string& text );

    const Cairo::Extent<false>& extent() const { return extent_; }

    void draw_centered_at( Cairo& cairo,
                           const double x,
                           const double y,
                           const double max_width = std::numeric_limits<double>::max() ) const;
    void draw_centered_rotated_at( Cairo& cairo, const double x, const double y ) const;

    operator const cairo_path_t*() const { return path_.get(); }
  };

  void set_font( const Font& font );
};

#endif /* CAIRO_OBJECTS_HH */
