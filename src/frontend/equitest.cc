
#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>
#include <thread>

#include <unistd.h>

#include "cairo_objects.hh"
#include "display.hh"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using namespace std::chrono;

#define SCREEN_RES_X 1920
#define SCREEN_RES_Y 1080
#define pi 3.14159265

void writePNGRaster(Raster420 & yuv_raster) {
  Cairo cairo { SCREEN_RES_X, SCREEN_RES_Y };

  /* open the PNG */
  PNGSurface png_image { "/home/brooke/repos/eyelink-latency/src/files/frame92_resized.png" };

  /* draw the PNG */
  cairo_identity_matrix( cairo );
  //cairo_scale( cairo, 0.234375, 0.263671875 );
  //cairo_scale( cairo, 0.5, 0.52734375 );
  cairo_scale(cairo, 1.0, 1.0);
  double center_x = 0, center_y = 0;
  cairo_device_to_user( cairo, &center_x, &center_y );
  cairo_translate( cairo, center_x, center_y );
  cairo_set_source_surface( cairo, png_image, 0, 0 );
  cairo_paint( cairo );

  /* finish and copy to YUV raster */
  cairo.flush();

  unsigned int stride = cairo.stride();
  for ( unsigned int y = 0; y < SCREEN_RES_Y; y++ ) {
    for ( unsigned int x = 0; x < SCREEN_RES_X; x++ ) {
      float red = cairo.pixels()[y * stride + 2 + ( x * 4 )] / 255.0;
      float green = cairo.pixels()[y * stride + 1 + ( x * 4 )] / 255.0;
      float blue = cairo.pixels()[y * stride + 0 + ( x * 4 )] / 255.0;

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
}

cv::Mat eul2rotm(double rotx, double roty, double rotz)
{
    cv::Mat R_x = (cv::Mat_<double>(3, 3) << 1, 0, 0,
                   0, cos(rotx), -sin(rotx),
                   0, sin(rotx), cos(rotx));

    cv::Mat R_y = (cv::Mat_<double>(3, 3) << cos(roty), 0, sin(roty),
                   0, 1, 0,
                   -sin(roty), 0, cos(roty));

    cv::Mat R_z = (cv::Mat_<double>(3, 3) << cos(rotz), -sin(rotz), 0,
                   sin(rotz), cos(rotz), 0,
                   0, 0, 1);
    cv::Mat K = (cv::Mat_<double>(3, 3) << 672, 0, SCREEN_RES_X / 2,
             0, 672, SCREEN_RES_Y / 2,
             0, 0, 1);

    cv::Mat R = R_z * R_y * R_x * K.inv();

    return R;
}

cv::Mat xyz_norm_map_x;
cv::Mat xyz_norm_map_y;

std::pair<double, double> reprojection(int x_img, int y_img, cv::Mat& img_src)
{
    cv::Mat xyz = (cv::Mat_<double>(3, 1) << (double)x_img, (double)y_img, 1);
    cv::Mat xyz_norm = xyz / norm(xyz);    

    //xyz_norm_map_x.at<cv::Vec3b>(x_img, y_img) = xyz_norm.at<double>(0,0);

    //std::cout << "[ " << xyz_norm.at<double>(0, 0) << ", " <<xyz_norm.at<double>(0, 1) << ", " <<xyz_norm.at<double>(0, 2) << " ]" << std::endl;

    cv::Mat Rot = eul2rotm(0.5, 0, 0);

    //std::cout << Rot << std::endl;

    cv::Mat ray3d = Rot * xyz_norm;

    //get 3d spherical coordinates
    double  xp = ray3d.at<double>(0, 0);
    double  yp = ray3d.at<double>(0, 1);
    double  zp = ray3d.at<double>(0, 2);
    //inverse formula for spherical projection, reference Szeliski book "Computer Vision: Algorithms and Applications" p439.
    double  theta = atan2(yp, sqrt(xp * xp + zp * zp));
    double  phi = atan2(xp, zp);

    //get 2D point on equirectangular map
    double x_sphere = (((phi * img_src.cols) / pi + img_src.cols) / 2);
    double y_sphere = (theta + pi / 2) * img_src.rows / pi;

    return std::make_pair(x_sphere, y_sphere);
}

void rectPortionRaster(Raster420 & yuv_raster, cv::Mat& img_src) {

  for ( unsigned int y = 0; y < SCREEN_RES_Y; y++ ) {
      for ( unsigned int x = 0; x < SCREEN_RES_X; x++ ) {
        //determine corresponding position in the equirectangular panorama
        std::pair<double, double> current_pos = reprojection(x, y, img_src);

        //extract the x and y value of the position in the equirect. panorama
        double current_x = current_pos.first;
        double current_y = current_pos.second;

        // if the current position exceeeds the panorama image limit -- leave pixel black and skip to next iteration
        if (current_x < 0 || current_y < 0 )
        {
            continue;
        }

        cv::Vec3b bgr = img_src.at<cv::Vec3b>(current_y, current_x);

        float blue = bgr.val[0]/255.0;
        float green = bgr.val[1]/255.0;
        float red = bgr.val[2]/255.0;

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
  return;
}

void onlineMethod() {
  cv::Mat img_src = cv::imread("/home/brooke/repos/eyelink-latency/src/files/frame92.png", cv::IMREAD_COLOR);
  VideoDisplay display { SCREEN_RES_X, SCREEN_RES_Y, false }; // fullscreen window @ 1920x1080 luma resolution
  Raster420 yuv_raster { SCREEN_RES_X  , SCREEN_RES_Y };
  rectPortionRaster(yuv_raster, img_src);
  Texture420 texture { yuv_raster };

  //std::cout << img_src.cols << std::endl;

  while (true) {
    display.draw( texture );
  }
}

void withShader() {

  VideoDisplay display { SCREEN_RES_X, SCREEN_RES_Y, false }; // fullscreen window @ 1920x1080 luma resolution
  Raster420 yuv_raster { SCREEN_RES_X, SCREEN_RES_Y };
  writePNGRaster(yuv_raster);
  Texture420 texture { yuv_raster };

  float roll = 0;
  float pitch = 0;
  float yaw = 0;

  while (true) {
    display.draw( texture );
    display.update_head_orientation( roll, pitch, yaw );
    roll += 0.001;
    pitch += 0.003;
    yaw += 0.001;
  }

}


int main()
{
  try {
    withShader();
    //onlineMethod();

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
