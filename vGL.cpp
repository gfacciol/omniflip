#include<sys/stat.h>
#include<stack>
#include <assert.h>
#include <cmath>
using std::isfinite;

#include <GLUT/glut.h>


extern "C" {
#include "iio.h"
}


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
   printf(" l : log of the range\n");
   printf(" s : save snapshot000.png\n");
   printf(" m : select the minimum of the range to move\n");
   printf(" M : select the Maximum of the range to move\n");
   printf(" q : quit\n");

}


/* GLOBAL VARIABLES */


const int GL_WIN_INITIAL_WIDTH = 800;
const int GL_WIN_INITIAL_HEIGHT = 600;
const int GL_WIN_INITIAL_X = 0;
const int GL_WIN_INITIAL_Y = 0;

//void *font = GLUT_BITMAP_TIMES_ROMAN_24;
//void *font = GLUT_BITMAP_9_BY_15;
void *font = GLUT_BITMAP_8_BY_13;

void outputText(float x, float y, char *string)
{
   int len, i;

   glRasterPos2f(x, y);
   len = (int) strlen(string);
   for (i = 0; i < len; i++) {
      glutBitmapCharacter(font, string[i]);
   }
}




typedef struct {
   float *x;
   int w;
   int h;
   int dim;
   char * filename;
} ImObj;

typedef struct {
   ImObj curr;
   ImObj alt;
   char ** filenames;
   int fileidx;
   int numfiles;
   float GSCALE;
   float GSHIFT;
   float GZOOM;
} ControllerObj;

ControllerObj Controller;




void image_range(ImObj U, float &vmin, float &vmax) {
   vmin =  1000000000;
   vmax = -1000000000;
   int i,j,c;
   for(j=0;j<U.dim *U.w*U.h; j++) {
      if(isfinite(U.x[j])){
         vmin = min(vmin,U.x[j]);
         vmax = max(vmax,U.x[j]);
      }
   }
   printf("Range:  %g %g \n",vmin, vmax);
}




void ControllerInit(int argc, char** argv) {
   int w, h, pixeldim;
   float *x;

   // controller INIT
   Controller.filenames = argv+1;
   Controller.numfiles = argc-1;
   Controller.fileidx = 0;

   /* load the initial image */
   char* filename = Controller.filenames[0];
   if ( file_exists(filename) ) {
      x = iio_read_image_float_vec(filename, &w, &h, &pixeldim);
      fprintf(stdout, "got a %dx%d image with %d channels\n", w, h, pixeldim);
   } 
   Controller.curr.x=x;
   Controller.curr.w =w;
   Controller.curr.h =h;
   Controller.curr.dim =pixeldim;
   Controller.curr.filename =filename;

   Controller.GSCALE=255;
   Controller.GSHIFT=0;
   Controller.GZOOM=1;

   float vmin, vmax;
   image_range(Controller.curr,vmin,vmax);
   Controller.GSCALE = vmax-vmin;
   Controller.GSHIFT = -vmin/Controller.GSCALE;

}



void display()
{
   ImObj current = Controller.curr;
   float zoom = Controller.GZOOM;


   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


   glMatrixMode (GL_PROJECTION);
   glLoadIdentity ();
   glOrtho (0, current.w*zoom, current.h*zoom, 0, -1, 1);
   glMatrixMode (GL_MODELVIEW);



   if(1){

      // TODO: THE TEXTURE DOES NOT NEED TO BE CREATED AND DESTROYED AT EACH DISPLAY CYCLE
      // allocate a texture name
      GLuint texture;
      glGenTextures( 1, &texture);

      // select our current texture
      glBindTexture( GL_TEXTURE_2D, texture );

      // select modulate to mix texture with color for shading
      glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
      // when texture area is small, bilinear filter the closest mipmap
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      // when texture area is large, bilinear filter the first mipmap
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


      float* data2   = current.x;
      int width      = current.w;
      int height     = current.h;

      // SCALE THE RANGE

      glPixelTransferf( GL_RED_BIAS,   Controller.GSHIFT);
      glPixelTransferf( GL_GREEN_BIAS, Controller.GSHIFT);
      glPixelTransferf( GL_BLUE_BIAS,  Controller.GSHIFT);

      glPixelTransferf( GL_RED_SCALE,   1./Controller.GSCALE);
      glPixelTransferf( GL_GREEN_SCALE, 1./Controller.GSCALE);
      glPixelTransferf( GL_BLUE_SCALE,  1./Controller.GSCALE);

      // build our texture mipmaps
      if(current.dim ==3)
           glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width,height, 0, GL_RGB, GL_FLOAT, data2);
//         gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height, GL_RGB , GL_FLOAT, data2 );
      if(current.dim ==1)
           glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width,height, 0, GL_LUMINANCE, GL_FLOAT, data2);
//         gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height, GL_LUMINANCE , GL_FLOAT, data2 );
      if(current.dim ==4)
           glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width,height, 0, GL_RGBA, GL_FLOAT, data2);
//         gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height, GL_RGBA , GL_FLOAT, data2 );
      if(current.dim ==2)
           glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width,height, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, data2);
 //        gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height, GL_LUMINANCE_ALPHA , GL_FLOAT, data2 );  



      glEnable( GL_TEXTURE_2D );
      glBegin( GL_QUADS );
      glColor3f(1.0, 1.0, 1.0);
      glTexCoord2d(0.0,0.0); glVertex3d(0,0,0);
      glTexCoord2d(1.0,0.0); glVertex3d(width *zoom,0,0);
      glTexCoord2d(1.0,1.0); glVertex3d(width *zoom ,height*zoom,0);
      glTexCoord2d(0.0,1.0); glVertex3d(0,height*zoom,0);
      glEnd();
      glDisable( GL_TEXTURE_2D );
      glDeleteTextures( 1, &texture );
   }

   // write text
   if(0){
      //  glClear(GL_COLOR_BUFFER_BIT);
      char teststr[1024];
      //glClearColor(0, 0, 0, 1);
      glColor3f(0.0, 1.0, 0.0);

      glPushMatrix();

      glTranslatef(-.5, -.5, 0);

      sprintf(teststr, "Keys 1 to 6 display objects");
      outputText(0.,0.,teststr);

      glPopMatrix();

   }


   glutSwapBuffers();
}

















void timer( int t ) {
   printf("%d\n", t);
}







// Window resize function
void glutResize(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

}



// Function that handles keyboard inputs
void glutKeyboard(unsigned char key, int x, int y)
{
   switch (key)
   {
      case 'h':
         printhelp();
         break;
      case 'q':
         exit(0);
         break;
      case 'c':
         float vmin, vmax;
         image_range(Controller.curr,vmin,vmax);
         Controller.GSCALE = vmax-vmin;
         Controller.GSHIFT = -vmin/Controller.GSCALE;
         glutPostRedisplay();
         break;

      case 'm':
         printf("moving the minimum\n");
         break;
      case 'M':
         printf("moving the Maximum\n");
         break;
      case '+': 
         if(Controller.GZOOM < 4) {

            Controller.GZOOM = Controller.GZOOM + .1;
            ImObj curr = Controller.curr;
            glutReshapeWindow(curr.w* Controller.GZOOM , curr.h* Controller.GZOOM );
            glutResize(curr.w* Controller.GZOOM , curr.h* Controller.GZOOM );

            glutPostRedisplay();


         }
         break;
      case '-': 
         if(Controller.GZOOM > .05){
            Controller.GZOOM = Controller.GZOOM - .1;
            ImObj curr = Controller.curr;

            glutReshapeWindow(curr.w* Controller.GZOOM , curr.h* Controller.GZOOM );
            glutResize(curr.w* Controller.GZOOM , curr.h* Controller.GZOOM );

            glutPostRedisplay();

         }
         break;

      case 8:                 // del, who knows?!
      case 127:                 // backspace


         if (Controller.fileidx==0) Controller.fileidx = Controller.numfiles-1;
         else Controller.fileidx = (Controller.fileidx-1) % Controller.numfiles;

         goto flipfiles;

         break;
      case ' ':

         Controller.fileidx = (Controller.fileidx+1) % Controller.numfiles;

flipfiles:
         ImObj curr = Controller.curr;
         ImObj alt = Controller.alt;

         if (alt.filename != Controller.filenames[Controller.fileidx]) {
            alt.filename = Controller.filenames[Controller.fileidx];

            // flip file patterns and load the image
            if(file_exists(alt.filename)) {

               if(alt.x) 
                  free(alt.x);

               int w,h,pixeldim;
               alt.x = iio_read_image_float_vec(alt.filename, &w, &h, &pixeldim);
               fprintf(stdout, "got a %dx%d image with %d channels\n", w, h, pixeldim);
               alt.w=w;
               alt.h=h;
               alt.dim=pixeldim;

               ImObj t = curr;
               curr = alt;
               alt = t;

            }

         }
         else{  // swap buffers
            // flip the internal images
            ImObj t = curr;
            curr = alt;
            alt = t;
         }

         Controller.curr = curr;
         Controller.alt = alt;

         //// keep the previous range
         {
            float vmin, vmax;
            image_range(Controller.curr,vmin,vmax);
            Controller.GSCALE = vmax-vmin;
            Controller.GSHIFT = -vmin/Controller.GSCALE;
         }

         glutReshapeWindow(curr.w* Controller.GZOOM , curr.h* Controller.GZOOM );
   glutTimerFunc(10,timer, 42);
         // glutResize(curr.w* Controller.GZOOM , curr.h* Controller.GZOOM );

         /*		glViewport(0, 0, curr.w, curr.h);
               glMatrixMode(GL_PROJECTION);
               glLoadIdentity();
               */		
         glutSetWindowTitle(curr.filename);

         display();
         glFlush();  		// Force writing of GL Commands
         glutPostRedisplay();

         break;



   }
   
}

// Function that handles mouse motion 
void glutMotion(int x, int y)
{
   printf("glutMotion: (%03d,%03d)\n", x,y);
}

// Function that handles mouse events (clicks)
void glutMouse(int button, int state, int x, int y)
{
   printf("glutMouse: button:%d state:%d\n  (%03d,%03d)",button, state, x,y);
   if(state)
      switch (button)
      {
         case 3:
            Controller.GSCALE +=10;
            glutPostRedisplay();

            break;
         case 4:
            Controller.GSCALE -=10;
            glutPostRedisplay();
            break;
      }
}





/* 
 * controller Object: 
 *  - var:  current_image
 *  - var:  image list
 *  - var:  image buffers
 *
 *  - init ( list of images)
 *  - float* current ()
 *  - float* next ()
 *  - sizeCurrent()
 *  - paramsCurrent()
 *
 * */


/* 
 * Visualization:
 *  - var: current texture
 *  - var: current zoom
 *
 * */
















int main(int argc, char **argv) {
   ControllerInit(argc, argv);



   /*
      Glut's initialization code. Set window's size and type of display.
      Window size is put half the 800x600 resolution as defined by above
      constants. The windows is positioned at the top leftmost area of
      the screen.
      */
   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA | GLUT_MULTISAMPLE );
   glutInitWindowPosition( GL_WIN_INITIAL_X, GL_WIN_INITIAL_Y );
   glutInitWindowSize( GL_WIN_INITIAL_WIDTH, GL_WIN_INITIAL_HEIGHT );

   int winNr= glutCreateWindow("OmniView");
   //glutFullScreen();

   /*
      The function below are called when the respective event
      is triggered.
      */
   glutReshapeFunc(glutResize);     // called every time  the screen is resized
   glutMouseFunc(glutMouse);        // called when the application receives a input from the mouse
   glutMotionFunc(glutMotion);      // called when the mouse moves over the screen with one of this button pressed
   glutPassiveMotionFunc(glutMotion); //  callback for a window is called when the mouse 
   //moves within the window while no mouse buttons are pressed.
//   	glutSpecialFunc(glutSpecial);    // called when a special key is pressed like SHIFT
   glutDisplayFunc(display);      	  // called when window needs to be redisplayed
   glutKeyboardFunc(glutKeyboard);    // called when the application receives a input from the keyboard

   /* 
      the callback function: load the frame and detect the faces and generate the view 
      */
   //glutIdleFunc(cvcallback); 
   //	glsceneinit();

   glutMainLoop();


   return 0;
}






