#include<sys/stat.h>
#include<stack>
#include <assert.h>
#include "CImg.h"
#include <cmath>
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
   return 0;
}


void printhelp() {
   printf(" <space>     : go to the next image\n");
   printf(" <backspace> : go to the previous image\n");
   printf(" scroll wheel: move boundary of the range\n");
   printf(" click       : inspect pixel values\n");
   printf(" c : reset contrast range\n");
   printf(" C : set/unset automatic contrast\n");
   printf(" l : log of the range\n");
   printf(" s : save snapshot000.png\n");
   printf(" m : select the minimum of the range to move\n");
   printf(" M : select the Maximum of the range to move\n");
   printf(" q : quit\n");

}

void image_range(CImg<float> &U, float &vmin, float &vmax) {
   vmin =  1000000000;
   vmax = -1000000000;
   int i,j,c;
   for(c=0;c<U.spectrum() ;c++) for(i=0;i<U.width() ;i++) for(j=0;j<U.height(); j++) {
      if(isfinite(U(i,j,0,c))){
         vmin = min(vmin,U(i,j,0,c));
         vmax = max(vmax,U(i,j,0,c));
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


void render_image(CImg<float> &U, float mrange, float Mrange, CImg<unsigned char> &disp) {
   int i,j,c;
   float scale_range = 255/(Mrange - mrange);

   for(c=0;c<min(U.spectrum(),3) ;c++) for(i=0;i<U.width() ;i++) for(j=0;j<U.height(); j++) {
      //      float curr = min(max(U(i,j,0,c),mrange),Mrange);
      float curr = (min(max(U(i,j,0,c),mrange),Mrange) -mrange) * scale_range;

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


/* GLOBAL VARIABLES */






int main(int argc,char **argv) {
   CImg<float> imageU, altimageU;
   int w, h, pixeldim;
   float vmax,vmin;

   // "GLOBAL" VARIABLES
   char **filenames=argv+1;
   int num_files=argc-1;
   int fileidx=0;
   char *filename, *altfilename=NULL;
   int AUTOMATIC_CONTRAST=0;

   int wheel=0;
   int my=-1;
   int mx=-1;
   int move_max=1;


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
   render_image(imageU,vmin,vmax,DISPimage);


   CImgDisplay main_disp(DISPimage,"Click a point",0);
   main_disp.set_title("Click a point %s",filename);
   main_disp.resize(imageU.width(), imageU.height());
   main_disp._wheel = wheel;

   printhelp();


   /* main loop */
   while (!main_disp.is_closed()) {
      main_disp.wait(main_disp);

      /* Wheel events control the image selection*/
      int new_wheel = main_disp.wheel();
      if ( new_wheel != wheel )  {

         float d= (vmax-vmin)*.2;
         if(move_max){
            if(new_wheel<wheel) {
               vmax-=d;
            } else {
               vmax+=d;
            }
         }
         else{
            if(new_wheel<wheel) {
               vmin-=d;
            } else {
               vmin+=d;
            }
         }
         wheel=new_wheel;
         {
            char str[1024];
            snprintf(str, 1024,"r:[%g,%g]", vmin,vmax);
            main_disp.set_title("%s",str);
         }

         render_image(imageU,vmin,vmax,DISPimage);
         DISPimage.display(main_disp);
      }
      DISPimage.display(main_disp);



      /* Button events control the selection of points */
//      if (main_disp.button()) { 
//      }


      /* Movement event */
      int nmx = main_disp.mouse_x();
      int nmy = main_disp.mouse_y();
      if (mx != nmx || my != nmy ){
         int x = nmx;
         int y = nmy;
         mx = nmx;
         my = nmy;
         char str[1024];

         if(x>=0 && y>=0 && x<imageU.width() && y<imageU.height()){
            if( imageU.spectrum()>1)
               snprintf(str,1024, "(%s) p:%d,%d v:[%g,%g,%g]",filename, x, y, imageU( x, y,0,0),  imageU( x, y,0,1),  imageU( x, y,0,2) );
            else
               snprintf(str,1024, "(%s) p:%d,%d v:[%g]", filename, x, y, imageU( x, y,0,0) );
         }
         main_disp.set_title("%s",str);
      }




      /* keyboard events */
      char key = main_disp.key();
      if ( key ) {
         switch( key ) {
            case 's':
               main_disp.set_title("SPLAT! Saving snapshot.png");
               DISPimage.save("snapshot.png");
               main_disp.set_title("SPLAT! snapshot.png saved");
               break;
            case 'c':
               image_range(imageU,vmin,vmax);
               render_image(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               break;
            case 'C':
               AUTOMATIC_CONTRAST=(AUTOMATIC_CONTRAST+1)%2;
               if(AUTOMATIC_CONTRAST) {
                  image_range(imageU,vmin,vmax);
                  render_image(imageU,vmin,vmax,DISPimage);
                  DISPimage.display(main_disp);
                  printf("Automatic contrast ON\n");
               }
               else 
                  printf("Automatic contrast OFF\n");
               break;
            case 'l':
               image_range(imageU,vmin,vmax);
               render_image_log(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               break;
            case 'q':
               exit(0);
               break;
            case 'm':
               main_disp.set_title("moving the minimum");
               printf("moving the minimum\n");
               move_max =0;
               break;
            case 'M':
               main_disp.set_title("moving the Maximum");
               printf("moving the Maximum\n");
               move_max =1;
               break;
            case 'h':
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
               main_disp.set_title("FLIP! %s",filename);
               if(AUTOMATIC_CONTRAST) image_range(imageU,vmin,vmax);
               render_image(imageU,vmin,vmax,DISPimage);
               DISPimage.display(main_disp);
               
               break;
            default:
               break;
         }
      }
   }
   return 0;
}
