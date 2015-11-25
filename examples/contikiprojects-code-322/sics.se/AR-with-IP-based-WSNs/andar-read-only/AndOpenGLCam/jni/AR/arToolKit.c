#include <GLES/gl.h>
#include <stdio.h>
#include <AR/ar.h>
#include <AR/param.h>
#include <../marker_info.h>
#include <android/log.h>
#include <stdlib.h>

#include <math.h>

#define DEBUG_LOGGING

int             xsize, ysize;
int             thresh = 100;
int             count = 0;

char           *cparam_name    = "/sdcard/andar/camera_para.dat";
ARParam         cparam;

//pattern-file
char           *patt_name      = "/sdcard/andar/patt.hiro";
//DY
char           *patt_name2      = "/sdcard/andar/patt.kanji";
char           *patt_name3      = "/sdcard/andar/4x4_89.patt";

int		cur_marker_id = -1;
int             patt_id;
//DY
int             patt_id2;
int             patt_id3;

double          patt_width     = 80.0;
double          patt_center[2] = {0.0, 0.0};
double          patt_trans[3][4];
//the opengl para matrix
extern float   gl_cpara[16];
float gl_para[16];

#define LIGHT_MAX_VALUE 600.0 //values over LIGHT_MAX -> 100% percentage
#define LIGHT_TEMP_MAX_SCALE 6.0
#define TEMP_MIN 18.0
#define TEMP_MAX 30.0
#define TEMP_RANGE (TEMP_MAX - TEMP_MIN)

#define ENERGY_MAX_VALUE 70.0
#define ENERGY_MAX_SCALE 6.0

enum E_COLORS{RED,GREEN,BLUE,YELLOW,CYAN,MAGENTA,WHITE};
//GLfloat colors[7][4] =
//{
//    1, 0, 0, 1,
//    0, 1, 0, 1,
//    0, 0, 1, 1,
//    1, 1, 0, 1,
//    0, 1, 1, 1,
//    1, 0, 1, 1,
//    1, 1, 1, 1
//};
GLfloat colors[7][4] =
{
    0.9f , 0.1f , 0.5f, 1, // cpu
    0.5f , 0.5f , 0.9f, 1, //listen
    0.1f , 0.9f , 0.1f, 1, //transmit
    1, 1, 0, 1,
    0, 1, 1, 1,
    1, 0, 1, 1,
    1, 1, 1, 1
};

#define SET_COLOR(X) glColor4f(colors[X][0], colors[X][1],colors[X][2],colors[X][3])

#define FAN_SECTIONS 50 //number of triangles to use to estimate a circle
#define NUM_OF_CIRCLE_VERTICES (FAN_SECTIONS+2)
GLfloat radius = 25.0f; //radius
GLfloat twoPi = 2.0f * 3.14159f;
GLfloat circle[2 * NUM_OF_CIRCLE_VERTICES]; //using 2d vertices

GLubyte circle_indices[FAN_SECTIONS*3];

void fill_circle_coords(void)
{
  int i;
  circle[0] = circle[1] = 0.0f; //root
  for(i = 0; i <= FAN_SECTIONS;i++)
  {
    circle[i*2+2] = radius * cos(i *  twoPi / FAN_SECTIONS);
    circle[i*2+3] = radius* sin(i * twoPi / FAN_SECTIONS);

    if ( i != FAN_SECTIONS )
    {
      circle_indices[i*3] = 0;
      circle_indices[i*3+1] = i+1;
      circle_indices[i*3+2] = i+2;
    }
  }
}

/*
 * Class:     edu_dhbw_andopenglcam_MarkerInfo
 * Method:    artoolkit_init
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_edu_dhbw_andopenglcam_MarkerInfo_artoolkit_1init
  (JNIEnv *env, jobject object, jint imageWidth, jint imageHeight, jint screenWidth, jint screenHeight) {
    ARParam  wparam;
	
    xsize = imageHeight;
    ysize = imageHeight;
    printf("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
	__android_log_write(ANDROID_LOG_ERROR,"AR","Camera parameter load error !!");
        printf("Camera parameter load error !!\n");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG_LOGGING
    else {
        __android_log_write(ANDROID_LOG_INFO,"AR","Camera parameter loaded successfully !!");
    }
#endif
    arParamChangeSize( &wparam, imageWidth, imageHeight, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

    static int patternsLoaded = 0;
    if ( !patternsLoaded )
    {
      if( (patt_id=arLoadPatt(patt_name)) < 0 ) {
    __android_log_write(ANDROID_LOG_ERROR,"AR","pattern load error !!");
          printf("pattern load error !!\n");
          exit(EXIT_FAILURE);
      }

      //DY
      if( (patt_id2=arLoadPatt(patt_name2)) < 0 ) {
        __android_log_write(ANDROID_LOG_ERROR,"AR","pattern2 load error !!");
              printf("pattern2 load error !!\n");
              exit(EXIT_FAILURE);
          }

      if( (patt_id3=arLoadPatt(patt_name3)) < 0 ) {
            __android_log_write(ANDROID_LOG_ERROR,"AR","pattern3 load error !!");
                  printf("pattern2 load error !!\n");
                  exit(EXIT_FAILURE);
              }

  #ifdef DEBUG_LOGGING
      else {
    __android_log_print(ANDROID_LOG_INFO,"AR","pattern loaded successfully!! id:%d", patt_id);
      }
  #endif

      patternsLoaded = 1;
    }

    //initialize openGL stuff
    argInit( &cparam, 1.0, 0, screenWidth, screenHeight, 0 );


    fill_circle_coords();
}

/*
 * Class:     edu_dhbw_andopenglcam_MarkerInfo
 * Method:    artoolkit_detectmarkers
 * Signature: ([B[D)I
 */
JNIEXPORT jint JNICALL Java_edu_dhbw_andopenglcam_MarkerInfo_artoolkit_1detectmarkers
  (JNIEnv *env, jobject object, jbyteArray image, jobject transMatMonitor) {
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    double 	    *matrixPtr;
    int             marker_num;
    int             j, k;

    //DY
    int found_pattern = 0;

    /* grab a vide frame */
    dataPtr = (*env)->GetByteArrayElements(env, image, JNI_FALSE);
    if( count == 0 ) arUtilTimerReset();
    count++;

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
	__android_log_write(ANDROID_LOG_ERROR,"AR","arDetectMarker failed!!");
        exit(EXIT_FAILURE);
    }

    /* check for object visibility */
    k = -1;
    for( j = 0; j < marker_num; j++ ) {
        if( patt_id == marker_info[j].id || patt_id2 == marker_info[j].id || patt_id3 == marker_info[j].id ) {
            if( k == -1 ) k = j;
            else if( marker_info[k].cf < marker_info[j].cf ) k = j;
        }
    }
    
    if ( k != -1 )
    {
      found_pattern = marker_info[k].id;
    }

#ifdef DEBUG_LOGGING
   __android_log_print(ANDROID_LOG_INFO,"AR","detected %d markers (%d)",marker_num, found_pattern );
#endif

    /* get the transformation between the marker and the real camera */
    arGetTransMat(&marker_info[k], patt_center, patt_width, patt_trans);

    //lock the matrix
    (*env)->MonitorEnter(env, transMatMonitor);
    cur_marker_id = k;
    argConvGlpara(patt_trans, gl_para);
    (*env)->MonitorExit(env, transMatMonitor);

    (*env)->ReleaseByteArrayElements(env, image, dataPtr, 0); 

    //DY
    //return k;
    return k != -1 ? found_pattern : -1;
    //return patt_id;
}
const float box[] =  {
			// FRONT
			-10.0f, -10.0f,  20.0f,
			 10.0f, -10.0f,  20.0f,
			-10.0f,  10.0f,  20.0f,
			 10.0f,  10.0f,  20.0f,
			// BACK
			-10.0f, -10.0f, 0.0f,
			-10.0f,  10.0f, 0.0f,
			 10.0f, -10.0f, 0.0f,
			 10.0f,  10.0f, 0.0f,
			// LEFT
			-10.0f, -10.0f,  20.0f,
			-10.0f,  10.0f,  20.0f,
			-10.0f, -10.0f, 0.0f,
			-10.0f,  10.0f, 0.0f,
			// RIGHT
			 10.0f, -10.0f, 0.0f,
			 10.0f,  10.0f, 0.0f,
			 10.0f, -10.0f, 20.0f,
			 10.0f,  10.0f, 20.0f,
			// TOP
			-10.0f,  10.0f,  20.0f,
			 10.0f,  10.0f,  20.0f,
			 -10.0f,  10.0f, 0.0f,
			 10.0f,  10.0f,  0.0f,
			// BOTTOM
			-10.0f, -10.0f,  20.0f,
			-10.0f, -10.0f, 0.0f,
			 10.0f, -10.0f, 20.0f,
			 10.0f, -10.0f, 0.0f,
		};
const float normals[] =  {
			// FRONT
			0.0f, 0.0f,  1.0f,
			0.0f, 0.0f,  1.0f,
			0.0f, 0.0f,  1.0f,
			0.0f, 0.0f,  1.0f,
			// BACK
			0.0f, 0.0f,  -1.0f,
			0.0f, 0.0f,  -1.0f,
			0.0f, 0.0f,  -1.0f,
			0.0f, 0.0f,  -1.0f,
			// LEFT
			-1.0f, 0.0f,  0.0f,
			-1.0f, 0.0f,  0.0f,
			-1.0f, 0.0f,  0.0f,
			-1.0f, 0.0f,  0.0f,
			// RIGHT
			1.0f, 0.0f,  0.0f,
			1.0f, 0.0f,  0.0f,
			1.0f, 0.0f,  0.0f,
			1.0f, 0.0f,  0.0f,
			// TOP
			0.0f, 1.0f,  0.0f,
			0.0f, 1.0f,  0.0f,
			0.0f, 1.0f,  0.0f,
			0.0f, 1.0f,  0.0f,
			// BOTTOM
			0.0f, -1.0f,  0.0f,
			0.0f, -1.0f,  0.0f,
			0.0f, -1.0f,  0.0f,
			0.0f, -1.0f,  0.0f,
		};
//Lighting variables
const static GLfloat   mat_ambient[]     = {0.0, 0.0, 1.0, 1.0};
const static GLfloat   mat_flash[]       = {0.0, 0.0, 1.0, 1.0};
const static GLfloat   mat_flash_shiny[] = {50.0};
const static GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
const static GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
const static GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};


//const GLfloat rectangle[] = {
//  -10.0f, -10.0f,
//   10.0f, -10.0f,
//  -10.0f,  10.0f,
//   10.0f,  10.0f
//};

const GLfloat square[] = {
  -50.0f, -50.0f,
   50.0f, -50.0f,
  -50.0f,  50.0f,
   50.0f,  50.0f
};

const GLfloat cube[] = {
   //bottom
   -10.0f, -10.0f, 0.0f, //a1

   10.0f, -10.0f, 0.0f, //b1
   10.0f, -10.0f, 0.0f, //b1

   10.0f,  10.0f, 0.0f, //c1
   10.0f,  10.0f, 0.0f, //c1

   -10.0f,  10.0f, 0.0f, //d1
   -10.0f,  10.0f, 0.0f, //d1

   -10.0f, -10.0f, 0.0f, //a1

   //up
   -10.0f, -10.0f, 20.0f, //a2

    10.0f, -10.0f, 20.0f, //b2
    10.0f, -10.0f, 20.0f, //b2

    10.0f,  10.0f, 20.0f, //c2
    10.0f,  10.0f, 20.0f, //c2

    -10.0f,  10.0f, 20.0f, //d2
    -10.0f,  10.0f, 20.0f, //d2

    -10.0f, -10.0f, 20.0f, //a2

    -10.0f, -10.0f, 0.0f, //a1
    -10.0f, -10.0f, 20.0f, //a2

    10.0f, -10.0f, 0.0f, //b1
    10.0f, -10.0f, 20.0f, //b2

    10.0f,  10.0f, 0.0f, //c1
    10.0f,  10.0f, 20.0f, //c2

    -10.0f,  10.0f, 0.0f, //d1
    -10.0f,  10.0f, 20.0f, //d2
};


void draw_rectangle( const float* vertices )
{
  glVertexPointer(2, GL_FLOAT, 0, vertices);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableClientState(GL_NORMAL_ARRAY);

}

void draw_wired_box( GLfloat scale )
{
  glPushMatrix();
  //color -> black wires
  glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
  glScalef(1.0f, 1.0f, scale );
  glVertexPointer(3, GL_FLOAT, 0, cube);

  glDrawArrays(GL_LINES, 0, 24);

  glDisableClientState(GL_NORMAL_ARRAY);
  glPopMatrix();
}

void draw_box( GLfloat scale )
{
  glPushMatrix();
  glScalef(1.0f, 1.0f, scale );

  glEnableClientState(GL_NORMAL_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, box);
  glNormalPointer(GL_FLOAT,0, normals);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
  glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
  glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
  glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
  glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);


  glDisableClientState(GL_NORMAL_ARRAY);

  glPopMatrix();
}

void draw_chart(GLint* percentages, GLint size)
{
  int whole = FAN_SECTIONS*3;
  int index_increment = 0;
  int i;
  int color_index = RED;
  int total_percent = 0;

  glVertexPointer(2, GL_FLOAT, 0, circle);
  for ( i=0; i<size ;i++ )
  {
    total_percent += percentages[i];
    if ( total_percent > FAN_SECTIONS )
    {
      total_percent = FAN_SECTIONS;
    }

    SET_COLOR(color_index);

    glDrawElements(GL_TRIANGLE_FAN, percentages[i]*3, GL_UNSIGNED_BYTE, circle_indices + index_increment);

    index_increment += percentages[i]*3;
    color_index++;
  }

  if ( total_percent < FAN_SECTIONS )
  {
    SET_COLOR(color_index);
    glDrawElements(GL_TRIANGLE_FAN, (FAN_SECTIONS - total_percent) *3, GL_UNSIGNED_BYTE, circle_indices + index_increment);
  }

//  //glDrawArrays(GL_TRIANGLE_FAN, 0, FAN_SECTIONS/2 + 2 ); //half
//
//  //glColor4ub(1,0,0,1);
//  SET_COLOR(YELLOW);
//  glDrawElements(GL_TRIANGLE_FAN, whole / 3, GL_UNSIGNED_BYTE, circle_indices);
//  index_increment += whole / 3;
//
//  SET_COLOR(CYAN);
//  glDrawElements(GL_TRIANGLE_FAN, whole / 3, GL_UNSIGNED_BYTE, circle_indices + index_increment );
//  index_increment += whole / 3;
//
//  SET_COLOR(RED);
//  glDrawElements(GL_TRIANGLE_FAN, whole / 3, GL_UNSIGNED_BYTE, circle_indices + index_increment );

//
//  // draw second half, range is 7 - 1 + 1 = 7 vertices
//  glDrawRangeElements(GL_QUADS, 1, 7, 12, GL_UNSIGNED_BYTE, indices+12);

}


#define DRAW_FLOAT_ARR_SIZE 6
/*
 * Class:     edu_dhbw_andopenglcam_MarkerInfo
 * Method:    draw
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_edu_dhbw_andopenglcam_MarkerInfo_draw
  (JNIEnv *env, jobject object, jfloatArray arr) {

  if(cur_marker_id != -1) {

    GLfloat light, temp, cpu, transmit, listen, totalEnergy;

    jfloat buf[DRAW_FLOAT_ARR_SIZE];

    (*env)->GetFloatArrayRegion(env, arr, 0, DRAW_FLOAT_ARR_SIZE, buf);
//    for (i = 0; i < DRAW_FLOAT_ARR_SIZE; i++) {
//       buf[i];
//    }
    light = buf[0];
    temp = buf[1];
    cpu = buf[2];
    listen = buf[3];
    transmit = buf[4];
    totalEnergy = buf[5];


    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf( gl_cpara );
    glClearDepthf( 1.0 );

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);


    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf( gl_para );


//    glEnable(GL_LIGHTING);
//    glEnable(GL_LIGHT0);
//    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
//    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
//    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
//    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
//    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);
//    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    draw_rectangle(square);

    glTranslatef( 0.0, 0.0, 2.0 ); //draw everything else a bit over the rectangle so that depth test works fine

    glTranslatef( -30.0, 20.0, 0.0 );
    if ( light != 0 )
    {
      GLfloat scale = 0;
      if ( light > LIGHT_MAX_VALUE ) light = LIGHT_MAX_VALUE;

      //LIGHT = yellow
      glColor4f(1.0f, 1.0f, 0.0f, 1.0f);

      scale = light/LIGHT_MAX_VALUE * LIGHT_TEMP_MAX_SCALE;
      draw_box(scale);
      draw_wired_box(scale);
      draw_wired_box(LIGHT_TEMP_MAX_SCALE);
    }

    glTranslatef( +30.0, 0.0, 0.0 );
    if ( temp != 0 )
    {
      GLfloat scale = 0;
      if ( temp < TEMP_MIN ) temp = TEMP_MIN;
      if ( temp > TEMP_MAX ) temp = TEMP_MAX;

      glColor4f(0.0f, 1.0f, 1.0f, 1.0f);

      scale = ((temp - TEMP_MIN) / TEMP_RANGE) * LIGHT_TEMP_MAX_SCALE;
      draw_box(scale);
      draw_wired_box(scale);
      draw_wired_box(LIGHT_TEMP_MAX_SCALE);
    }

    glTranslatef( +30.0, 0.0, 0.0 );
    if ( totalEnergy != 0 )
    {
      GLfloat scale = 0;

      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

      scale = totalEnergy/ENERGY_MAX_VALUE * ENERGY_MAX_SCALE;
      draw_box(scale);
      draw_wired_box(scale);
      draw_wired_box(ENERGY_MAX_SCALE);
    }

    //chart
    glTranslatef( -20.0, -40.0, 0.0 );
    if ( totalEnergy != 0 )
    {
      GLint percentages[3];
      percentages[0] = (cpu/totalEnergy) * FAN_SECTIONS;
      percentages[1] = (listen/totalEnergy) * FAN_SECTIONS;
      //percentages[2] = (transmit/totalEnergy) * 100;

#ifdef DEBUG_LOGGING
      __android_log_print(ANDROID_LOG_INFO,"Energyyy","Percent %d-%d", percentages[0], percentages[1]);
#endif

      draw_chart(percentages,2);
    }

    //draw cube

    //glTranslatef( 0.0, 0.0, 12.5 );

    //draw_box(box);

    //glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
    //draw_box(buf[1]);


    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_DEPTH_TEST);

  }
}


