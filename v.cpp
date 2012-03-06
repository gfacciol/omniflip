#include<sys/stat.h>
#include<stack>
#include <assert.h>
#include "CImg.h"
#include <cmath>
#include "libgen.h"
using std::isfinite;

extern "C" {
#include "iio.h"
}

//	g++ -O3 -Wall -L/usr/lib -L/usr/X11R6/lib -lpng -lfreetype -lSM -lICE -lXext -lX11 -lz -lm -ldl -lpthread flowviewer.cpp  -o flowViewer

using namespace cimg_library;

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))


int file_exists (char * fileName)
{
   struct stat buf;
   int i = stat ( fileName, &buf );
   /* File found */
   if ( i == 0 ) { return 1; }
   if (fileName[0] == '-') {return 1; } /* stdin */
   return 0;
}


void printhelp() {
   printf(" <space>     : go to the next image\n");
   printf(" <backspace> : go to the previous image\n");
   printf(" scroll wheel: move center of the image range\n");
   printf(" <shift>+scroll wheel: change width of the image range\n");
   printf(" left click  : distance tool\n");
   printf(" c : reset contrast range\n");
   printf(" C : set/unset automatic contrast\n");
   printf(" l : log of the range\n");
   printf(" s : save snapshot000.png\n");
   printf(" q : quit\n");

}

void image_range(CImg<float> &U, float &vmin, float &vmax) {
   vmin =  1000000000;
   vmax = -1000000000;
   int i,j,c;
   for(c=0;c<U.spectrum() ;c++) for(i=0;i<U.width() ;i++) for(j=0;j<U.height(); j++) {
      if(isfinite(U(i,j,0,c))){
         vmin = min(U(i,j,0,c),vmin);
         vmax = max(U(i,j,0,c),vmax);
      }
   }
   printf("Range:  %g %g \n",vmin, vmax);
}


void float_to_image(float * x, int w, int h, int s, CImg<float> &U) {
   int i,j,c;
   U.resize(w,h,1,s);
   for(c=0;c<s ;c++) for(i=0;i<w ;i++) for(j=0;j<h; j++) {
      U(i,j,0,c) = x[(i+j*w)*s + c];
   }
}



void render_image_log(CImg<float> &U, float mrange, float Mrange, CImg<unsigned char> &disp) {
   int i,j,c;
   float scale_range = 255./log(Mrange-mrange+1);
   for(c=0;c<U.spectrum() ;c++) for(i=0;i<U.width() ;i++) for(j=0;j<U.height(); j++) {
      float curr = log(min(max(U(i,j,0,c),mrange),Mrange)-mrange+1)*scale_range;

      if(U.spectrum()<3) {
         disp(i,j,0,0) = (unsigned char) curr;
         disp(i,j,0,1) = (unsigned char) curr;
         disp(i,j,0,2) = (unsigned char) curr;
      }
      else {
         disp(i,j,0,c) = (unsigned char) curr;
      }
   }



}



int colorremap[3] = {0,1,2};

void render_image(CImg<float> &U, float mrange, float Mrange, CImg<unsigned char> &disp) {
   int i,j,c;
   float scale_range = 255/(Mrange - mrange);
   float curr;

//   // validate the current remap
//   for (c=0;c<3; c++) 
//      colorremap[c] = c % U.spectrum();

   // for each color
   for(c=0;c<3 ;c++) {
      // compute new color, with safety check
      int newc = colorremap[c] % U.spectrum();
      for(i=0;i<U.width() ;i++) for(j=0;j<U.height(); j++) {
         if(newc>=0)     // reserve the negative indices for NONE 
            curr = (min(max(U(i,j,0,newc),mrange),Mrange) -mrange) * scale_range;
         else 
            curr =0;
         disp(i,j,0,c) = (unsigned char) curr;
      }
   }
}

/* GLOBAL VARIABLES */



int main(int argc,char **argv) {
   CImg<float> imageU, altimageU;
   int w, h, pixeldim;
   float vmax,vmin;           // max and min value of the current display range 
   float image_max,image_min; // max and min of the current image

   // "GLOBAL" VARIABLES
   char **filenames=argv+1;
   int num_files=argc-1;
   int fileidx=0;
   char *filename, *altfilename=NULL;
   int AUTOMATIC_CONTRAST=0;

   int wheel=0;
   int my=-1;
   int mx=-1;

   int dragging=0;
   int px0=0,py0=0;


   if(argc<=1) {
      printf("Usage: %s files\n", argv[0]);
      printf("   view the images \n" );
      exit(1);
   }

   filename = argv[1];

   /* SET AN INITIAL IMAGE SIZE */
   imageU.resize(100,100,1,3);


   /* load the initial image */
   if ( file_exists(filename) ) {
      float *x = iio_read_image_float_vec(filename, &w, &h, &pixeldim);
      if (x) {
         fprintf(stdout, "got a %dx%d image with %d channels\n", w, h, pixeldim);
         float_to_image(x,w,h,pixeldim,imageU);
         free(x);
      }else {
        printf("%s is a file \n", filename);
      }
   } else {
      printf("initial file not found\n");
      exit(1);
   }


   CImg<unsigned char> DISPimage (imageU.width(), imageU.height(), 1, 3 ); // initialize the disp image
   image_range(imageU,vmin,vmax);
   image_max = vmax;
   image_min = vmin;

   render_image(imageU,vmin,vmax,DISPimage);


   // hidden profile display
   CImgDisplay profile_disp(500,200,"Color profile of the X-axis",0,false,true);


   // main display
   CImgDisplay main_disp(DISPimage,"Click a point",0);
   main_disp.set_title("Click a point %s",basename(filename));
   main_disp.resize(imageU.width(), imageU.height());
   main_disp._wheel = wheel;

   printhelp();



   /* main loop */
   while (!main_disp.is_closed()) {
      main_disp.wait(main_disp);

      /* Wheel events control the image selection*/
      int new_wheel = main_disp.wheel();
      if ( new_wheel != wheel )  {

         float v_center = (vmax+vmin)/2;
         float v_radius = (vmax-vmin)/2;

         // modify the radius of the range
         if(main_disp.is_keySHIFTLEFT() || main_disp.is_keySHIFTRIGHT())
         {
            float d=  (vmax-vmin)*.1;
            if(new_wheel<wheel) {
               v_radius-=d;
            } else {
               v_radius+=d;
            }
            v_radius=max(v_radius,0);
         }
         // modify the center of the range
         else
         {
            float d= (vmax-vmin)*.1;
            if(new_wheel<wheel) {
               if(vmax-d>image_min) // bound
                  v_center-=d;
            } else {
               if(vmin+d<image_max) // bound
                  v_center+=d;
            }
         }
         vmax=v_center+v_radius;
         vmin=v_center-v_radius;

         wheel=new_wheel;

         // display some info int the title bar
         {
            char str[1024];
            snprintf(str, 1024,"r:[%g,%g]", vmin,vmax);
            main_disp.set_title("%s",str);
         }

         // recompute the image with the new range
         render_image(imageU,vmin,vmax,DISPimage);

         //DISPimage.display(main_disp);
         // display the range data in the upper left corner of the image.
         const unsigned char green[] = { 10,255,20 };
         CImg<int> tmp(DISPimage);
         tmp.draw_text(0,0,"DISPLAY RANGE \ncenter val. %.2f \nradius %.2f",green,0,1,15, v_center, v_radius).display(main_disp);

      }


      /* Movement event */
      int nmx = main_disp.mouse_x();
      int nmy = main_disp.mouse_y();
      if (mx != nmx || my != nmy ){
         int x = nmx;
         int y = nmy;
         mx = nmx;
         my = nmy;

         // IF WE ARE INSIDE THE IMAGE
         if(x>=0 && y>=0 && x<imageU.width() && y<imageU.height()){
            char str[1024];
            if( imageU.spectrum()>1) {
               if(imageU.spectrum()==2)
                  snprintf(str,1024, "(%03d,%03d): %g,%g :%s ",x, y, imageU( x, y,0,0),  imageU( x, y,0,1) ,basename(filename)  );
               else if(imageU.spectrum()==3)
                  snprintf(str,1024, "(%03d,%03d): %g,%g,%g :%s ",x, y, imageU( x, y,0,0),  imageU( x, y,0,1),  imageU( x, y,0,2),basename(filename)  );
               else if(imageU.spectrum()>=4)
                  snprintf(str,1024, "(%03d,%03d): %g,%g,%g,%g :%s ",x, y, imageU( x, y,0,0),  imageU( x, y,0,1),  imageU( x, y,0,2), imageU( x, y,0,3),basename(filename)  );
            }
            else
               snprintf(str,1024, "(%03d,%03d): %g :%s", x, y, imageU( x, y,0,0) ,basename(filename));

            // UPDATE TITLE
            main_disp.set_title("%s",str);


            // IF SHIFT IS PRESSED modify the center of the range with the value of the current pixel
            if(main_disp.is_keySHIFTLEFT() || main_disp.is_keySHIFTRIGHT())
            {
               float v_center = (vmax+vmin)/2;
               float v_radius = (vmax-vmin)/2;

               // new center value
               // v_center = imageU( x, y,0,0);
               int cw=1;
               v_center = imageU.get_crop (x-cw, y-cw, x+cw, y+cw).mean(); //smooth movement

               vmax=v_center+v_radius;
               vmin=v_center-v_radius;

               // recompute the image with the new range
               render_image(imageU,vmin,vmax,DISPimage);

               //DISPimage.display(main_disp);
               // display the range data in the upper left corner of the image.
               const unsigned char green[] = { 10,255,20 };
               CImg<int> tmp(DISPimage);
               tmp.draw_text(0,0,"DISPLAY RANGE \ncenter val. %.2f \nradius %.2f",green,0,1,15, v_center, v_radius).display(main_disp);

            }
            else
            {
               DISPimage.display(main_disp);
            }

         }
      }


      /* Button events for line selection*/
      if (main_disp.button()&1) {
         // start dragging
         if (dragging==0) { 
            px0=nmx;
            py0=nmy;
         } 
         const unsigned char green[] = { 10,255,20 };
         CImg<int> tmp(DISPimage);
         tmp.draw_line(px0,py0,nmx,nmy,green).draw_text((px0+nmx)/2,(py0+nmy)/2,"%.2f",green,0,1,15,hypot (px0-nmx, py0-nmy)).display(main_disp);
         dragging=1;
      } else {
         // end dragging
         if (dragging==1) {
         }
         dragging=0;
      }



      if(! profile_disp.is_closed()) {
         int x=nmx;
         int y=nmy;
         if( x>=0 && y>=0 && x<imageU.width() && y<imageU.height()) {
         unsigned long hatch = 0xF0F0F0F0;
         const unsigned char
            red[]   = { 255,0,0 },
            green[] = { 0,255,0 },
            blue [] = { 0,0,255 },
            black[] = { 0,0,0 };
 //        const unsigned int
 //           val_red   = imageU(x,y,0),
 //                     val_green = imageU(x,y,1),
 //                     val_blue  = imageU(x,y,2);

         // Create and display the image of the intensity profile
         CImg<unsigned char>(profile_disp.width(),profile_disp.height(),1,3,255).
            draw_grid(-50*100.0f/imageU.width(),-50*100.0f/256,0,0,false,false,black,0.2f,0xCCCCCCCC,0xCCCCCCCC).
//            draw_axes(0,imageU.width()-1.0f,255.0f,0.0f,black).
            draw_graph(imageU.get_shared_line(y,0,0),red,1,1,0,vmin,vmax,1).
//            draw_graph(imageU.get_shared_line(y,0,1),green,1,1,0,vmin,vmax,1).
//            draw_graph(imageU.get_shared_line(y,0,2),blue,1,1,0,vmin,vmax,1).
//            draw_text(30,5,"Pixel (%d,%d)={%d %d %d}",black,0,1,13, nmx,nmy,val_red,val_green,val_blue).
 //           draw_line(x,0,x,profile_disp.height()-1,black,0.5f,hatch=cimg::rol(hatch)).
            display(profile_disp);
         }
      }




      /* keyboard events */
      char key = main_disp.key();
      if ( key ) {
         switch( key ) {
            case 'r': case 'R':
               colorremap[0] = (colorremap[0]+2)%(imageU.spectrum()+1)-1;
               printf("channel %d -> color %d\n", colorremap[0],0);
               render_image(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               break;
            case 'g': case 'G':
               colorremap[1] = (colorremap[1]+2)%(imageU.spectrum()+1)-1;
               printf("channel %d -> color %d\n", colorremap[1],1);
               render_image(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               break;
            case 'b': case 'B':
               colorremap[2] = (colorremap[2]+2)%(imageU.spectrum()+1)-1;
               printf("channel %d -> color %d\n", colorremap[2],2);
               render_image(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               break;
            case 's': case 'S':
               main_disp.set_title("SPLAT! Saving snapshot.png");
               DISPimage.save("snapshot.png");
               main_disp.set_title("SPLAT! snapshot.png saved");
               break;
            case 'p':
               profile_disp.move(main_disp.window_x() + imageU.width(),main_disp.window_y());
               profile_disp.show();
               main_disp.close();
               main_disp.show();
               profile_disp.flush();
               main_disp.flush();
               break;
            case 'c':
               image_range(imageU,vmin,vmax);
               image_max = vmax;
               image_min = vmin;
               render_image(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               break;
            case 'C':
               AUTOMATIC_CONTRAST=(AUTOMATIC_CONTRAST+1)%2;
               if(AUTOMATIC_CONTRAST) {
                  image_range(imageU,vmin,vmax);
                  image_max = vmax;
                  image_min = vmin;
                  render_image(imageU,vmin,vmax,DISPimage);
                  DISPimage.display(main_disp);
                  printf("Automatic contrast ON\n");
               }
               else 
                  printf("Automatic contrast OFF\n");
               break;
            case 'l': case 'L':
               image_range(imageU,vmin,vmax);
               image_max = vmax;
               image_min = vmin;
               render_image_log(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               break;
            case 'q': case 'Q':
               exit(0);
               break;
            case 'h': case 'H':
               printhelp();
               break;

            case 8:                 // backspace

               if (fileidx==0) fileidx = num_files-1;
               else fileidx = (fileidx-1) % num_files;

               goto flipfiles;

               break;
            case ' ':

               fileidx = (fileidx+1) % num_files;

flipfiles:
               if (altfilename != filenames[fileidx]) {
                  altfilename = filenames[fileidx];

                  // flip file patterns and load the image
                  if(file_exists(altfilename)) {
                     altimageU = imageU;

                     char *ts;
                     ts          = filename;
                     filename    = altfilename;
                     altfilename = ts;

                     float *x = iio_read_image_float_vec(filename, &w, &h, &pixeldim);
                     if (x) {
                        fprintf(stdout, "got a %dx%d image with %d channels\n", w, h, pixeldim);
                        float_to_image(x,w,h,pixeldim,imageU);
                        free(x);
                     }else {
                        printf("%s is a file \n", filename);
                     }
                  }

               }
               else{  // swap buffers
                  // flip the internal images
                  imageU.swap(altimageU);

                  char *ts;
                  ts          = filename;
                  filename    = altfilename;
                  altfilename = ts;
               }

               //// keep the previous range
               // image_range(imageU,vmin,vmax);
               if(imageU.width() != DISPimage.width() || imageU.height() != DISPimage.height()) {
                  DISPimage.resize(imageU.width(), imageU.height());
                  main_disp.resize(imageU.width(), imageU.height());
               }
               main_disp.set_title("FLIP! %s",basename(filename));
               if(AUTOMATIC_CONTRAST) {
                  image_range(imageU,vmin,vmax);
                  image_max = vmax;
                  image_min = vmin;
               }
               render_image(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               
               // force the re-display of pixel information
               mx =-1;

               break;
            default:
               break;
         }
      }
   }
   return 0;
}
