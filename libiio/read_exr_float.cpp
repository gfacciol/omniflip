#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "iio.h"
#define IIO_MAX_DIMENSION 5
#define IIO_TYPE_FLOAT 7
struct iio_image {
   int dimension;        // 1, 2, 3 or 4
   int sizes[IIO_MAX_DIMENSION];
   int pixel_dimension;
   int type;             // IIO_TYPE_*

   int meta;             // IIO_META_*
   int format;           // IIO_FORMAT_*

   bool contiguous_data;
   bool caca[3];
   void *data;
};

#include <ImfCRgbaFile.h>
#include <ImfInputFile.h>
#include <ImfFrameBuffer.h>

extern "C" { 


   namespace Imf{
      int read_whole_exr(struct iio_image *x, char *filename)
      {
         // Open EXR file

         Imf::InputFile file(filename);

         // Get image dimensions.

         Imath::Box2i dw = file.header().dataWindow();

         int width = dw.max.x - dw.min.x + 1;
         int height = dw.max.y - dw.min.y + 1;

         // Allocate memory to read image bits. We will only try to read R, G and B
         // here, but additional channels like A (alpha) could also be added...

         struct sRGB
         {
            float r, g, b;
         };

         sRGB *pixels = new sRGB[width * height];

         // Now create the frame buffer to feed the image reader with. We will use
         // the Slice method flexibility to directly read R, G and B data in an
         // interlaced manner, using appropriate x & y stride values.

         Imf::FrameBuffer frameBuffer;

         frameBuffer.insert("R", Slice(Imf::FLOAT, (char*)(&pixels[0].r), sizeof(sRGB), width * sizeof(sRGB)));
         frameBuffer.insert("G", Slice(Imf::FLOAT, (char*)(&pixels[0].g), sizeof(sRGB), width * sizeof(sRGB)));
         frameBuffer.insert("B", Slice(Imf::FLOAT, (char*)(&pixels[0].b), sizeof(sRGB), width * sizeof(sRGB)));

         file.setFrameBuffer(frameBuffer);
         file.readPixels(dw.min.y, dw.max.y);


         float *finaldata = (float*) malloc(3*width*height*sizeof(float));
         int i;
         for (i=0; i<(width*height);i++) {
            sRGB pix = pixels[ i ];
            finaldata[3*i+0] = pix.r;
            finaldata[3*i+1] = pix.g;
            finaldata[3*i+2] = pix.b;
            //IIO_DEBUG("read %d rgb %g %g %g\n", i, finaldata[3*i+0], finaldata[3*i+1], finaldata[3*i+2]);
         }
         // fill struct fields
         x->dimension = 2;
         x->sizes[0] = width;
         x->sizes[1] = height;
         x->pixel_dimension = 3;
         x->type = IIO_TYPE_FLOAT;
         x->format = x->meta = -42;
         x->data = finaldata;
         x->contiguous_data = false;
         return 0;
      }
   }
}
