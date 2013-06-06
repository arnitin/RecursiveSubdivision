
/*  
    smooth.c
    Nate Robins, 1998

    Model viewer program.  Exercises the glm library.
*/

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <GL/glut.h>
#include "gltb.h"
#include "glm.h"
#include "dirent32.h"

#pragma comment( linker, "/entry:\"mainCRTStartup\"" )  // set the entry point to be main()
#define DATA_DIR "data/"

#define T(x) (model->triangles[(x)])

GLfloat   newVertices[1000000];            /* array of vertices  */
GLfloat * oldVertices;
GLMtriangle * oldTriangles;
int       start = 0;

#define TNew(x) (newTriangles[(x)])

void build_data_structure();
void newTriangle(double * a,double * b,double * c,int i,GLMtriangle* newTriangles,GLfloat* newVerticesIn) ;
void newDrawMechanism();
void find_even_neighbors(double x,double y,double z,double * res) ;
void the_division_process(/*GLMtriangle * newTrianglesIn,*/GLfloat* newVertices);
void calculate_even(double *a,int i,GLMtriangle * newTrianglesIn,GLfloat* newVerticesIn,int k);
void find_odd_neighbors(double * x, double * y ,double * res);
void calculate_odd(double *a,double *b,int i,GLMtriangle * newTrianglesIn,GLfloat* newVerticesIn,int k);
void crossProduct(double* v1,double* v2, GLfloat *result);
void Minus( GLfloat* v1,GLfloat* v2, double* result);

char*      model_file = NULL;		/* name of the obect file */
GLuint     model_list = 0;		    /* display list for object */
GLMmodel*  model;			        /* glm model data structure */
GLfloat    scale;			        /* original scale factor */
GLfloat    smoothing_angle = 90.0;	/* smoothing angle */
GLfloat    weld_distance = 0.00001;	/* epsilon for welding vertices */
GLboolean  facet_normal = GL_FALSE;	/* draw with facet normal? */
GLboolean  bounding_box = GL_FALSE;	/* bounding box on? */
GLboolean  performance = GL_FALSE;	/* performance counter on? */
GLboolean  stats = GL_FALSE;		/* statistics on? */
GLuint     material_mode = 0;		/* 0=none, 1=color, 2=material */
GLint      entries = 0;			    /* entries in model menu */
GLdouble   pan_x = 0.0;
GLdouble   pan_y = 0.0;
GLdouble   pan_z = 0.0;

#include <time.h>
#if defined(_WIN32)
#include <sys/timeb.h>
#define CLK_TCK 1000
#else
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#endif
float
elapsed(void)
{
  static long begin = 0;
  static long finish, difference;
    
#if defined(_WIN32)
  static struct timeb tb;
  ftime(&tb);
  finish = tb.time*1000+tb.millitm;
#else
  static struct tms tb;
  finish = times(&tb);
#endif
    
  difference = finish - begin;
  begin = finish;
    
  return (float)difference/(float)CLOCKS_PER_SEC;
}

void
shadowtext(int x, int y, char* s) 
{
  int lines;
  char* p;
    
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 
	  0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glColor3ub(0, 0, 0);
  glRasterPos2i(x+1, y-1);
  for(p = s, lines = 0; *p; p++) {
    if (*p == '\n') {
      lines++;
      glRasterPos2i(x+1, y-1-(lines*18));
    }
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
  }
  glColor3ub(0, 128, 255);
  glRasterPos2i(x, y);
  for(p = s, lines = 0; *p; p++) {
    if (*p == '\n') {
      lines++;
      glRasterPos2i(x, y-(lines*18));
    }
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
  }
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glEnable(GL_DEPTH_TEST);
}

void
lists(void)
{
  GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
  GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
  GLfloat specular[] = { 0.0, 0.0, 0.0, 1.0 };
  GLfloat shininess = 65.0;
    
  glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
  glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
  glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    
  if (model_list)
    glDeleteLists(model_list, 1);
    
  /* generate a list */
  if (material_mode == 0) { 
    if (facet_normal)
      model_list = glmList(model, GLM_FLAT);
    else
      model_list = glmList(model, GLM_SMOOTH);
  } else if (material_mode == 1) {
    if (facet_normal)
      model_list = glmList(model, GLM_FLAT | GLM_COLOR);
    else
      model_list = glmList(model, GLM_SMOOTH | GLM_COLOR);
  } else if (material_mode == 2) {
    if (facet_normal)
      model_list = glmList(model, GLM_FLAT | GLM_MATERIAL);
    else
      model_list = glmList(model, GLM_SMOOTH | GLM_MATERIAL);
  }
}

void
init(void)
{
  gltbInit(GLUT_LEFT_BUTTON);
    
  /* read in the model */
  model = glmReadOBJ(model_file);
  scale = glmUnitize(model);
  glmFacetNormals(model);
  glmVertexNormals(model, smoothing_angle);
    
  if (model->nummaterials > 0)
    material_mode = 2;
    
  /* create new display lists */
  lists();
  /*  GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
  
   glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);     
  glEnable(GL_DEPTH_TEST);     
  glEnable(GL_CULL_FACE);
*/

  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mat_shininess[] = { 50.0 };
  GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
  glClearColor (0.0, 0.0, 0.0, 0.0);
  glShadeModel (GL_SMOOTH);
  
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);


}

void
reshape(int width, int height)
{
  gltbReshape(width, height);
    
  glViewport(0, 0, width, height);
    
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -3.0);
}


#define NUM_FRAMES 5
void display(void)
{
  static char s[256], t[32];
  static char* p;
  static int frames = 0;
    
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
  glPushMatrix();
    
  glTranslatef(pan_x, pan_y, 0.0);
    
  gltbMatrix();
  if(!start) {
#if 0   // glmDraw() performance test 
    if (material_mode == 0) { 
      if (facet_normal)
	glmDraw(model, GLM_FLAT);
      else
	glmDraw(model, GLM_SMOOTH);
    } else if (material_mode == 1) {
      if (facet_normal)
	glmDraw(model, GLM_FLAT | GLM_COLOR);
      else
	glmDraw(model, GLM_SMOOTH | GLM_COLOR);
    } else if (material_mode == 2) {
      if (facet_normal)
	glmDraw(model, GLM_FLAT | GLM_MATERIAL);
      else
	glmDraw(model, GLM_SMOOTH | GLM_MATERIAL);
    }
#else
    glCallList(model_list);
#endif
  
    glDisable(GL_LIGHTING);
    if (bounding_box) {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glEnable(GL_CULL_FACE);
      glColor4f(1.0, 0.0, 0.0, 0.25);
      glutSolidCube(2.0);
      glDisable(GL_BLEND);
    }
  
  } else {

    //glDisable(GL_LIGHTING);

    newDrawMechanism();
    glEnable(GL_LIGHTING);
  }  
  glPopMatrix();
    
  if (stats) {
    /* XXX - this could be done a _whole lot_ faster... */
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    glColor3ub(0, 0, 0);
    sprintf(s, "%s\n%d vertices\n%d triangles\n%d normals\n"
            "%d texcoords\n%d groups\n%d materials",
            model->pathname, model->numvertices, model->numtriangles, 
            model->numnormals, model->numtexcoords, model->numgroups,
            model->nummaterials);
    shadowtext(5, height-(5+18*1), s);
  }
    
  /* spit out frame rate. */
  frames++;
  if (frames > NUM_FRAMES) {
    sprintf(t, "%g fps", frames/elapsed());
    frames = 0;
  }
  if (performance) {
    shadowtext(5, 5, t);
  }
    
  glutSwapBuffers();
  glEnable(GL_LIGHTING);
}

void
keyboard(unsigned char key, int x, int y)
{
  GLint params[2];
    
  switch (key) {
  case 'h':
    printf("help\n\n");
    printf("w         -  Toggle wireframe/filled\n");
    printf("c         -  Toggle culling\n");
    printf("n         -  Toggle facet/smooth normal\n");
    printf("b         -  Toggle bounding box\n");
    printf("r         -  Reverse polygon winding\n");
    printf("m         -  Toggle color/material/none mode\n");
    printf("p         -  Toggle performance indicator\n");
    printf("s/S       -  Scale model smaller/larger\n");
    printf("t         -  Show model stats\n");
    printf("o         -  Weld vertices in model\n");
    printf("+/-       -  Increase/decrease smoothing angle\n");
    printf("W         -  Write model to file (out.obj)\n");
    printf("q/escape  -  Quit\n\n");
    break;
        
  case 't':
    stats = !stats;
    break;
        
  case 'p':
    performance = !performance;
    break;
        
  case 'm':
    material_mode++;
    if (material_mode > 2)
      material_mode = 0;
    printf("material_mode = %d\n", material_mode);
    lists();
    break;
        
  case 'd':
    glmDelete(model);
    init();
    lists();
    break;
        
  case 'w':
    glGetIntegerv(GL_POLYGON_MODE, params);
    if (params[0] == GL_FILL)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


    break;
        
  case 'c':
    if (glIsEnabled(GL_CULL_FACE))
      glDisable(GL_CULL_FACE);
    else
      glEnable(GL_CULL_FACE);
    break;
        
  case 'b':
    bounding_box = !bounding_box;
    break;
        
  case 'n':
    facet_normal = !facet_normal;
    lists();
    break;
        
  case 'r':
    glmReverseWinding(model);
    lists();
    break;
        
  case 's':
    glmScale(model, 0.8);
    lists();
    break;
        
  case 'S':
    glmScale(model, 1.25);
    lists();
    break;
        
  case 'o':
    //printf("Welded %d\n", glmWeld(model, weld_distance));
    glmVertexNormals(model, smoothing_angle);
    lists();
    break;
        
  case 'O':
    weld_distance += 0.01;
    printf("Weld distance: %.2f\n", weld_distance);
    glmWeld(model, weld_distance);
    glmFacetNormals(model);
    glmVertexNormals(model, smoothing_angle);
    lists();
    break;
        
  case '-':
    smoothing_angle -= 1.0;
    printf("Smoothing angle: %.1f\n", smoothing_angle);
    glmVertexNormals(model, smoothing_angle);
    lists();
    break;
        
  case '+':
    smoothing_angle += 1.0;
    printf("Smoothing angle: %.1f\n", smoothing_angle);
    glmVertexNormals(model, smoothing_angle);
    lists();
    break;
        
  case 'W':
    glmScale(model, 1.0/scale);
    glmWriteOBJ(model, "out.obj", GLM_SMOOTH | GLM_MATERIAL);
    break;
        
  case 'R':
    {
      GLuint i;
      GLfloat swap;
      for (i = 1; i <= model->numvertices; i++) {
	swap = model->vertices[3 * i + 1];
	model->vertices[3 * i + 1] = model->vertices[3 * i + 2];
	model->vertices[3 * i + 2] = -swap;
      }
      glmFacetNormals(model);
      lists();
      break;
    }
        
  case 27:
    exit(0);
    break;
  case 'y' : 

    //           model->numtriangles = 1;

    build_data_structure(); 
    
    //    model->numtriangles =6;
    //if(!start)
    start++;
      
      
  }
    
  glutPostRedisplay();
}

void
menu(int item)
{
  int i = 0;
  DIR* dirp;
  char* name;
  struct dirent* direntp;
    
  if (item > 0) {
    keyboard((unsigned char)item, 0, 0);
  } else {
    dirp = opendir(DATA_DIR);
    while ((direntp = readdir(dirp)) != NULL) {
      if (strstr(direntp->d_name, ".obj")) {
	i++;
	if (i == -item)
	  break;
      }
    }
    if (!direntp)
      return;
    name = (char*)malloc(strlen(direntp->d_name) + strlen(DATA_DIR) + 1);
    strcpy(name, DATA_DIR);
    strcat(name, direntp->d_name);
    model = glmReadOBJ(name);
    scale = glmUnitize(model);
    glmFacetNormals(model);
    glmVertexNormals(model, smoothing_angle);
        
    if (model->nummaterials > 0)
      material_mode = 2;
    else
      material_mode = 0;
        
    lists();
    free(name);
        
    glutPostRedisplay();
  }
}

static GLint      mouse_state;
static GLint      mouse_button;

void
mouse(int button, int state, int x, int y)
{
  GLdouble model[4*4];
  GLdouble proj[4*4];
  GLint view[4];
    
  /* fix for two-button mice -- left mouse + shift = middle mouse */
  if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
    button = GLUT_MIDDLE_BUTTON;
    
  gltbMouse(button, state, x, y);
    
  mouse_state = state;
  mouse_button = button;
    
  if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON) {
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);
    gluProject((GLdouble)x, (GLdouble)y, 0.0,
	       model, proj, view,
	       &pan_x, &pan_y, &pan_z);
    gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
		 model, proj, view,
		 &pan_x, &pan_y, &pan_z);
    pan_y = -pan_y;
  }
    
  glutPostRedisplay();
}

void
motion(int x, int y)
{
  GLdouble model[4*4];
  GLdouble proj[4*4];
  GLint view[4];
    
  gltbMotion(x, y);
    
  if (mouse_state == GLUT_DOWN && mouse_button == GLUT_MIDDLE_BUTTON) {
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);
    gluProject((GLdouble)x, (GLdouble)y, 0.0,
	       model, proj, view,
	       &pan_x, &pan_y, &pan_z);
    gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
		 model, proj, view,
		 &pan_x, &pan_y, &pan_z);
    pan_y = -pan_y;
  }
    
  glutPostRedisplay();
}

int
main(int argc, char** argv)
{
  int buffering = GLUT_DOUBLE;
  struct dirent* direntp;
  DIR* dirp;
  int models;
    
  glutInitWindowSize(512, 512);
  glutInit(&argc, argv);
    
  while (--argc) {
    if (strcmp(argv[argc], "-sb") == 0)
      buffering = GLUT_SINGLE;
    else
      model_file = argv[argc];
  }
    
  if (!model_file) {
    model_file = "data/torus.obj";
  }
    
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | buffering);
  glutCreateWindow("Loop Subdivision");
    
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
    
  models = glutCreateMenu(menu);
  dirp = opendir(DATA_DIR);
  if (!dirp) {
    fprintf(stderr, "%s: can't open data directory.\n", argv[0]);
  } else {
    while ((direntp = readdir(dirp)) != NULL) {
      if (strstr(direntp->d_name, ".obj")) {
	entries++;
	glutAddMenuEntry(direntp->d_name, -entries);
      }
    }
    closedir(dirp);
  }
    
  glutCreateMenu(menu);
  glutAddMenuEntry("Smooth", 0);
  glutAddMenuEntry("", 0);
  glutAddSubMenu("Models", models);
  glutAddMenuEntry("", 0);
  glutAddMenuEntry("[w]   Toggle wireframe/filled", 'w');
  glutAddMenuEntry("[c]   Toggle culling on/off", 'c');
  glutAddMenuEntry("[n]   Toggle face/smooth normals", 'n');
  glutAddMenuEntry("[b]   Toggle bounding box on/off", 'b');
  glutAddMenuEntry("[p]   Toggle frame rate on/off", 'p');
  glutAddMenuEntry("[t]   Toggle model statistics", 't');
  glutAddMenuEntry("[m]   Toggle color/material/none mode", 'm');
  glutAddMenuEntry("[r]   Reverse polygon winding", 'r');
  glutAddMenuEntry("[s]   Scale model smaller", 's');
  glutAddMenuEntry("[S]   Scale model larger", 'S');
  glutAddMenuEntry("[o]   Weld redundant vertices", 'o');
  glutAddMenuEntry("[+]   Increase smoothing angle", '+');
  glutAddMenuEntry("[-]   Decrease smoothing angle", '-');
  glutAddMenuEntry("[W]   Write model to file (out.obj)", 'W');
  glutAddMenuEntry("", 0);
  glutAddMenuEntry("[Esc] Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    
  init();
  glutMainLoop();
  return 0;
}

void triangleCoord(int v1,int v2,int v3,double * A,double *B, double * C,int flag) {
  int vIndices[3];
  vIndices[0] = v1;
  vIndices[1] = v2;
  vIndices[2] = v3;
  GLfloat * ptrtoVert;

  GLdouble modelProj[4*4];
  GLdouble proj[4*4];
  GLint    view[4];
  double   tx[3], ty[3], tz[3];
  int mul = 1;

  if(!start && (flag == 0)){
    ptrtoVert = model->vertices;
    mul = 3;
  } else if(flag == 0){
    ptrtoVert = newVertices;
    mul = 1;
  } else {
    ptrtoVert = oldVertices;
    mul = 1;
  }
 

  A[0] = ptrtoVert[mul*vIndices[0] + 0];
  A[1] = ptrtoVert[mul*vIndices[0] + 1];
  A[2] = ptrtoVert[mul*vIndices[0] + 2];

  B[0] = ptrtoVert[mul*vIndices[1] + 0];
  B[1] = ptrtoVert[mul*vIndices[1] + 1];
  B[2] = ptrtoVert[mul*vIndices[1] + 2];

  C[0] = ptrtoVert[mul*vIndices[2] + 0];
  C[1] = ptrtoVert[mul*vIndices[2] + 1];
  C[2] = ptrtoVert[mul*vIndices[2] + 2];

 
}

void findMidPoints(double * one,double * two,double * out) {
  out[0] = (one[0] + two[0])/2;
  out[1] = (one[1] + two[1])/2;
  out[2] = (one[2] + two[2])/2;  
}

void build_data_structure(){
  int     i,j,v1Index,v2Index,v3Index;
  double  a[3],b[3],c[3];
  double  midAB[3], midBC[3],midCA[3];
  GLfloat newVerticesLoc[1000000]; 
  GLMtriangle * newTriangles;       /* array of triangles */
  double res[3];

  
  memset(newVerticesLoc, 0, sizeof(GLfloat)*1000000);
  int temp,k;
  newTriangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) * 4 * model->numtriangles);

  oldVertices = (GLfloat*)malloc(sizeof(newVertices));
  memcpy(oldVertices,&newVertices,sizeof(newVertices));

  oldTriangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) *  (model->numtriangles));
  memcpy(oldTriangles,&model->triangles,sizeof(model->triangles));
  
  if(!start){
    for (i = 0; i <model->numtriangles; i++) {
      v1Index = (float)T(i).vindices[0];
      v2Index = (float)T(i).vindices[1];
      v3Index = (float)T(i).vindices[2];
    
      triangleCoord((int)v1Index,(int)v2Index,(int)v3Index,a,b,c,0);  
      newTriangle(a,b,c,i,oldTriangles,oldVertices);
    }
  } else {
    for (i = 0; i <model->numtriangles; i++) {
      oldTriangles[i].vindices[0] = model->triangles[i].vindices[0];
      oldTriangles[i].vindices[1] = model->triangles[i].vindices[1];
      oldTriangles[i].vindices[2] = model->triangles[i].vindices[2];

    }    
  }

  for (i = 0; i <model->numtriangles; i++) {
    v1Index = (float)T(i).vindices[0];
    v2Index = (float)T(i).vindices[1];
    v3Index = (float)T(i).vindices[2];
    
    triangleCoord((int)v1Index,(int)v2Index,(int)v3Index,a,b,c,0);
    findMidPoints(a,b,midAB);
    findMidPoints(b,c,midBC);
    findMidPoints(c,a,midCA);

   

    newTriangle(a,midAB,midCA,4*i+0,newTriangles,newVerticesLoc);
    calculate_even( a,4*i,newTriangles,newVerticesLoc,0);
    calculate_odd(a,b,4*i,newTriangles,newVerticesLoc,1);
    calculate_odd(c,a,4*i,newTriangles,newVerticesLoc,2);

    
    
    newTriangle(midAB,b,midBC,4*i+1,newTriangles,newVerticesLoc);
    calculate_even( b,4*i+1,newTriangles,newVerticesLoc,1);
    calculate_odd(a,b,4*i+1,newTriangles,newVerticesLoc,0);
    calculate_odd(b,c,4*i+1,newTriangles,newVerticesLoc,2);
    
    newTriangle(midCA,midBC,c,4*i+2,newTriangles,newVerticesLoc);
    calculate_even( c,4*i+2,newTriangles,newVerticesLoc,2);
    calculate_odd(c,a,4*i+2,newTriangles,newVerticesLoc,0);
    calculate_odd(b,c,4*i+2,newTriangles,newVerticesLoc,1);



    newTriangle(midAB,midBC,midCA,4*i+3,newTriangles,newVerticesLoc);
    calculate_odd(a,b,4*i+3,newTriangles,newVerticesLoc,0);
    calculate_odd(b,c,4*i+3,newTriangles,newVerticesLoc,1);
    calculate_odd(c,a,4*i+3,newTriangles,newVerticesLoc,2);


  }

  model->numtriangles = 4*model->numtriangles;
  // the_division_process(newVerticesLoc);


  memcpy(&newVertices,&newVerticesLoc,sizeof(newVerticesLoc));
  memcpy(&model->triangles,&newTriangles,sizeof(newTriangles));
}





void calculate_odd(double *a,double *b,int i,GLMtriangle * newTrianglesIn,GLfloat* newVerticesIn,int k){
  int    j = 3*i;
  int    temp = 3*j + k*3;
  double res[3];
  res[0] = res[1] = res[2] = 0;

  
  res[0] += (double)3/8*a[0];
  res[1] += (double)3/8*a[1];
  res[2] += (double)3/8*a[2];

  res[0] += (double)3/8*b[0];
  res[1] += (double)3/8*b[1];
  res[2] += (double)3/8*b[2];

  find_odd_neighbors(a,b,res);
  if(res[0] == -11){
    res[0] = res[1] = res[2] = 0;
    res[0] += (double)1/2*a[0];
    res[1] += (double)1/2*a[1];
    res[2] += (double)1/2*a[2];
    
    res[0] += (double)1/2*b[0];
    res[1] += (double)1/2*b[1];
    res[2] += (double)1/2*b[2];
    
  }

  newVerticesIn[temp + 0] = res[0];
  newVerticesIn[temp + 1] = res[1];
  newVerticesIn[temp + 2] = res[2];
}  

void find_odd_neighbors(double * x, double * y ,double * res){
  double  a[3],b[3],c[3];
  int i,v1Index,v2Index,v3Index;
  int k = 0;
  
  for(i=0;i<model->numtriangles;i++){
    v1Index = (float)oldTriangles[(i)].vindices[0];
    v2Index = (float)oldTriangles[(i)].vindices[1];
    v3Index = (float)oldTriangles[(i)].vindices[2];
    
    triangleCoord((int)v1Index,(int)v2Index,(int)v3Index,a,b,c,1);
    
    if((a[0] == x[0] && a[1] == x[1] && a[2] == x[2]) &&
       (b[0] == y[0] && b[1] == y[1] && b[2] == y[2])) {
      
      res[0] += (double)1/8*c[0];
      res[1] += (double)1/8*c[1];
      res[2] += (double)1/8*c[2];
      k++;
    } else if((b[0] == x[0] && b[1] == x[1] && b[2] == x[2]) &&
	      (a[0] == y[0] && a[1] == y[1] && a[2] == y[2])) {
      
      res[0] += (double)1/8*c[0];
      res[1] += (double)1/8*c[1];
      res[2] += (double)1/8*c[2];
      k++;
    }else if((b[0] == x[0] && b[1] == x[1] && b[2] == x[2]) &&
	     (c[0] == y[0] && c[1] == y[1] && c[2] == y[2])) {
	
      res[0] += (double)1/8*a[0];
      res[1] += (double)1/8*a[1];
      res[2] += (double)1/8*a[2];
      k++;
    }else if((c[0] == x[0] && c[1] == x[1] && c[2] == x[2]) &&
	     (b[0] == y[0] && b[1] == y[1] && b[2] == y[2])) {
      
      res[0] += (double)1/8*a[0];
      res[1] += (double)1/8*a[1];
      res[2] += (double)1/8*a[2];
      k++;
    }else if((c[0] == x[0] && c[1] == x[1] && c[2] == x[2]) &&
	     (a[0] == y[0] && a[1] == y[1] && a[2] == y[2])) {
      
      res[0] += (double)1/8*b[0];
      res[1] += (double)1/8*b[1];
      res[2] += (double)1/8*b[2];
      k++;
    }else if((a[0] == x[0] && a[1] == x[1] && a[2] == x[2]) &&
	     (c[0] == y[0] && c[1] == y[1] && c[2] == y[2])) {
      
      res[0] += (double)1/8*b[0];
      res[1] += (double)1/8*b[1];
      res[2] += (double)1/8*b[2];
      k++;
    }
  }
  if(k ==1 ){
    //    res[0] = res[1] = res[2] = -11;
    //    printf("sdf");
  }
}
void  newTriangle(double * a,double * b,double * c,int i,GLMtriangle * newTrianglesIn,GLfloat* newVerticesIn){
  double * ptrtoArr;
  int      j = 3*i;
  int      k,temp,ju;
   

  // change for midpoint

  for(k=0;k<3;k++){
    temp = 3*j + k*3;
    if(k == 0) { 
      ptrtoArr = a;
    } else if(k == 1){
      ptrtoArr = b; 
    } else {
      ptrtoArr = c;
    }

    if(temp == 576){
      ju++;
    }
    
    newTrianglesIn[(i)].vindices[k] = temp;
    newVerticesIn[temp]     = ptrtoArr[0];
    newVerticesIn[temp + 1] = ptrtoArr[1];
    newVerticesIn[temp + 2] = ptrtoArr[2];
  }
}


void newDrawMechanism(){
  int i = 0,v1Index,v2Index,v3Index;
  GLfloat  a[3],b[3],c[3];
  double   u[3],v[3];
  GLfloat  normal[3];
  GLfloat  color[3] = {2.9f,1.9f,3.9f}; 
    
  
  glShadeModel(GL_FLAT);
   glEnable(GL_COLOR_MATERIAL);
  
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mat_shininess[] = { 5.0 };
  GLfloat light_position[] = { -1.0, 1.0, 1.0, 0.0 };
  glClearColor (0.0, 0.0, 0.0, 0.0);
  // glShadeModel (GL_SMOOTH);
    glShadeModel(GL_FLAT);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
  
    /*
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
    */
  glColor3fv(color);
  glBegin(GL_TRIANGLES);
  
  for (i = 0; i <model->numtriangles; i++) {
    v1Index = /*model->triangles[(i)].vindices[0];*/(float)T(i).vindices[0];
    v2Index = /*newTriangles[(i)].vindices[1];*/(float)T(i).vindices[1];
    v3Index = /*newTriangles[(i)].vindices[2];*/(float)T(i).vindices[2];


    a[0] = newVertices[v1Index + 0] ;  a[1] = newVertices[v1Index + 1] ; a[2] = newVertices[v1Index + 2] ; 
    b[0] = newVertices[v2Index + 0] ;  b[1] = newVertices[v2Index + 1] ; b[2] = newVertices[v2Index + 2] ; 
    c[0] = newVertices[v3Index + 0] ;  c[1] = newVertices[v3Index + 1] ; c[2] = newVertices[v3Index + 2] ; 
      
    Minus(b,a,u);
    Minus(c,a,v);
    crossProduct(u,v,normal);

    glNormal3fv(normal);
    glColor3fv(color);
    glVertex3f(newVertices[v1Index + 0], newVertices[v1Index + 1],newVertices[v1Index + 2]);
    glVertex3f(newVertices[v2Index + 0], newVertices[v2Index + 1],newVertices[v2Index + 2]);
    glVertex3f(newVertices[v3Index + 0], newVertices[v3Index + 1],newVertices[v3Index + 2]);
  } 
  glEnd();       
}

 
void calculate_even(double *a,int i,GLMtriangle * newTrianglesIn,GLfloat* newVerticesIn,int k){
  int      j = 3*i;
  int temp = 3*j + k*3;
  double res[3];
  res[0] = res[1] = res[2] = 0;

  find_even_neighbors(a[0],a[1],a[2],res);
  
  newVerticesIn[temp + 0] = res[0];
  newVerticesIn[temp + 1] = res[1];
  newVerticesIn[temp + 2] = res[2];
}


void find_even_neighbors(double x,double y,double z,double * res) {
  double  a[3],b[3],c[3];
  int i,v1Index,v2Index,v3Index;
  int k = 0;
  double sixteen = (double)1/16;
  double temper[20][3];

  for(i=0;i<model->numtriangles;i++){
    v1Index = (float)oldTriangles[(i)].vindices[0]; //OLD TRIANGL!!
    v2Index = (float)oldTriangles[(i)].vindices[1];
    v3Index = (float)oldTriangles[(i)].vindices[2];
    
    triangleCoord((int)v1Index,(int)v2Index,(int)v3Index,a,b,c,1);
    
    if(a[0] == x && a[1] == y && a[2] == z){
      res[0] += sixteen * b[0];// + sixteen * c[0];
      res[1] += sixteen * b[1];// + sixteen * c[1];
      res[2] += sixteen * b[2];// + sixteen * c[2];

      temper[k][0] = b[0];// 	temper[2*k + 1][0] = c[0]; 
      temper[k][1] = b[1];//  temper[2*k + 1][1] = c[1]; 
      temper[k][2] = b[2];//  temper[2*k + 1][2] = c[2]; 
      k++;
    } else if(b[0] == x && b[1] == y && b[2] == z){
      res[0] += sixteen * c[0];// + sixteen * c[0];
      res[1] += sixteen * c[1];// + sixteen * c[1];
      res[2] += sixteen * c[2];// + sixteen * c[2];

      temper[k][0] = c[0];// 	temper[2*k + 1][0] = c[0]; 
      temper[k][1] = c[1];//  temper[2*k + 1][1] = c[1]; 
      temper[k][2] = c[2];//  temper[2*k + 1][2] = c[2]; 
      k++;
    } else if (c[0] == x && c[1] == y && c[2] == z){
      res[0] += sixteen * a[0];// + sixteen * b[0];
      res[1] += sixteen * a[1];// + sixteen * b[1];
      res[2] += sixteen * a[2];// + sixteen * b[2];

      temper[k][0] = a[0]; //	temper[2*k + 1][0] = b[0]; 
      temper[k][1] = a[1]; // temper[2*k + 1][1] = b[1]; 
      temper[k][2] = a[2]; // temper[2*k + 1][2] = b[2]; 
      k++;
    }
  }
  // because repeating each vertex

  if(k!=6){
      
    double a1 = 3 + 2*(cos(2*3.14/k));
    double b1 = (a1*a1)/64;
    double c1 = double(5)/8;
    double beta =  c1 - b1;
    double betaByN = beta/k;

    if(k == 2) {
      //   betaByN = (double)1/8;
      //    beta =  (double)1/4;
    }
    res[0] = res[1] = res[2] = 0;
    for(i=0;i<k;i++) {
      res[0] += temper[i][0] * betaByN;// + temper[2*i + 1][0]*betaByN;
      res[1] += temper[i][1] * betaByN;// + temper[2*i + 1][1]*betaByN;
      res[2] += temper[i][2] * betaByN;// + temper[2*i + 1][2]*betaByN;
    }
       
    res[0] += (1-beta) * x;
    res[1] += (1-beta) * y;
    res[2] += (1-beta) * z;
      
      
  } else {    
    /*
      res[0] /= 2;
      res[1] /= 2;
      res[2] /= 2; 
    */
    res[0] += (double)10/16*x;
    res[1] += (double)10/16*y;
    res[2] += (double)10/16*z;   
      
  }       
}

void crossProduct(double* v1,double* v2, GLfloat *result){
  
  result[0] = v1[1]*v2[2] - v2[1]*v1[2];
  result[1] = (v1[0]*v2[2] - v2[0]*v1[2]) * (-1);
  result[2] = v1[0]*v2[1] - v2[0]*v1[1];
}

void Minus( GLfloat* v1,GLfloat* v2, double* result) {
  result[0] = (v1[0] - v2[0]);
  result[1] = (v1[1] - v2[1]);
  result[2] = (v1[2] - v2[2]);
}
