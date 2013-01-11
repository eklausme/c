/* Mix of SunEarthMoon.c + opengl1gd.c
   Gordian Edenhofer
   Elmar Klausmeier, 22-Oct-2012

   Under Linux compile with
	cc -Wall `pkg-config --cflags gtk+-2.0 gtkglext-1.0` openglSunEM.c -o openglSunEM `pkg-config --libs gtk+-2.0 gtkglext-1.0`
   Run with
	./openglSunEM
   Stop with ESC, or Alt-F4, stop motion with Space bar.
   Arrow keys increase or decrease rotation speed.
   Mouse moves whole scene (arcball).
*/

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <png.h>
#include "ArcBall2.c"


int debug=0, textureFlag=0;
static GLfloat spin = 0.0;
static GLfloat colv[4] = { 1.0, 0.3, 0.7, 0.2 };
static int rotat=1;
GLfloat	xrotS		=  0.0f;
GLfloat	yrotS		=  0.0f;
GLfloat	xrotSspeed	=  0.0f;
GLfloat	yrotSspeed	=  1.0f;
GLfloat	xrot		=  0.0f;	// X Rotation
GLfloat	yrot		=  0.0f;	// Y Rotation
GLfloat	xrotspeed	=  0.0f;	// X Rotation Speed
GLfloat	yrotspeed	=  0.0f;	// Y Rotation Speed
GLfloat	xrotM		=  0.0f;	// X Rotation
GLfloat	yrotM		=  0.0f;	// Y Rotation
GLfloat	xrotMspeed	=  0.0f;	// X Rotation Speed
GLfloat	yrotMspeed	=  0.0f;	// Y Rotation Speed
GLfloat	zoom		= -30.0f;	// Depth Into The Screen
GLfloat	height		=  0.0f;	// Height Of Ball From Floor

GLUquadricObj	*e, *m, *b, *s;		// Quadric for drawing a sphere

Matrix4fT Transform = {{ 1.0f,  0.0f,  0.0f,  0.0f,				// Final Transform
                         0.0f,  1.0f,  0.0f,  0.0f,
                         0.0f,  0.0f,  1.0f,  0.0f,
                         0.0f,  0.0f,  0.0f,  1.0f }};
Matrix3fT LastRot = {{ 1.0f,  0.0f,  0.0f,					// Last Rotation
                       0.0f,  1.0f,  0.0f,
                       0.0f,  0.0f,  1.0f }};

Matrix3fT ThisRot = {{ 1.0f,  0.0f,  0.0f,					// This Rotation
                       0.0f,  1.0f,  0.0f,
                       0.0f,  0.0f,  1.0f }};
ArcBall_t ArcBall;

// Light Parameters
static GLfloat LightAmb[] = {1.0f, 1.0f, 1.0f, 1.0f};	// Ambient Light (wenn der Z-Wert auf 10 gesetzt wird entsteht eine blaue Beleuchtung)
static GLfloat LightDif[] = {1.0f, 1.0f, 1.0f, 1.0f};	// Diffuse Light
static GLfloat LightPos[] = {0.0f, 0.0f, 0.0f, 1.0f};	// Light Position

GLuint texture[4];					// 4 textures

#define GLERR()		{GLenum e=glGetError(); if(e!=GL_NO_ERROR) {printf("file=%s,line=%d: %s\n",__FILE__,__LINE__,gluErrorString(e));}}


/* From http://blog.nobel-joergensen.com/2010/11/07/loading-a-png-as-texture-in-opengl-using-libpng/
*/
unsigned char *loadPng(char *name, int *outWidth, int *outHeight, int *outHasAlpha) {
    unsigned char *outData;
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    int i;
    FILE *fp;

    if ((fp = fopen(name, "rb")) == NULL)
        return NULL;

    /* Create and initialize the png_struct
     * with the desired error handler
     * functions.  If you want to use the
     * default stderr and longjump method,
     * you can supply NULL for the last
     * three parameters.  We also supply the
     * the compiler header file version, so
     * that we know if the application
     * was compiled with a compatible version
     * of the library.  REQUIRED
     */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
            NULL, NULL, NULL);

    if (png_ptr == NULL) {
        fclose(fp);
        return NULL;
    }

    /* Allocate/initialize the memory
     * for image information.  REQUIRED. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
        return NULL;
    }

    /* Set error handling if you are
     * using the setjmp/longjmp method
     * (this is the normal method of
     * doing things with libpng).
     * REQUIRED unless you  set up
     * your own error handlers in
     * the png_create_read_struct()
     * earlier.
     */
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* Free all of the memory associated
         * with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        fclose(fp);
        /* If we get here, we had a
         * problem reading the file */
        return NULL;
    }

    /* Set up the output control if
     * you are using standard C streams */
    png_init_io(png_ptr, fp);

    /* If we have already
     * read some of the signature */
    png_set_sig_bytes(png_ptr, sig_read);

    /*
     * If you have enough memory to read
     * in the entire image at once, and
     * you need to specify only
     * transforms that can be controlled
     * with one of the PNG_TRANSFORM_*
     * bits (this presently excludes
     * dithering, filling, setting
     * background, and doing gamma
     * adjustment), then you can read the
     * entire image (including pixels)
     * into the info structure with this
     * call
     *
     * PNG_TRANSFORM_STRIP_16 |
     * PNG_TRANSFORM_PACKING  forces 8 bit
     * PNG_TRANSFORM_EXPAND forces to
     *  expand a palette into RGB
     */
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, png_voidp_NULL);

    *outWidth = info_ptr->width;
    *outHeight = info_ptr->height;
    switch (info_ptr->color_type) {
        case PNG_COLOR_TYPE_RGBA:
            *outHasAlpha = TRUE;
            break;
        case PNG_COLOR_TYPE_RGB:
            *outHasAlpha = FALSE;
            break;
        default:
            printf("color_type=%d not supported\n",info_ptr->color_type); 
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return NULL;
    }
    unsigned int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    outData = (unsigned char*) malloc(row_bytes * (*outHeight));
    //outData = (unsigned char*) calloc(row_bytes * (*outHeight),1);

    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

    for (i=0; i<*outHeight; ++i) {
        // note that png is ordered top to
        // bottom, but OpenGL expect it bottom to top
        // so the order or swapped
        memcpy(outData+(row_bytes * (*outHeight-1-i)), row_pointers[i], row_bytes);
    }

    /* Clean up after the read, and free any memory allocated */
    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

    fclose(fp); // Close the file
    //printf("loadPng(): outWidth=%d, outHeight=%d, outHasAlpha=%d, name=%s\n",
    //  *outWidth, *outHeight, *outHasAlpha, name);

    return outData; // That's it
}

static void endGtkGL() {
	glDeleteTextures(4,texture);
	printf("texture[0..3] = (%d %d %d %d)\n",texture[0],texture[1],texture[2],texture[3]);
	gtk_main_quit();
}

/* From http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
*/
unsigned char *loadBMP (const char *fn, int *width, int *height) {
	unsigned char header[54];
	FILE *fp;
	*width = 0;  *height = 0;

	if ((fp = fopen(fn,"rb")) == NULL) {
		printf("Cannot open %s\n", fn);
		return NULL;
	}
	if (fread(header,1,54,fp) != 54) {
		printf("%s does not have 54 byte long header\n",fn);
		return NULL;
	}
	if ( header[0] != 'B' || header[1] != 'M' ){
		printf("%s: not a correct BMP file\n",fn);
		return NULL;
	}
	int dataPos   = *(int*)&(header[0x0A]);
	int imageSize = *(int*)&(header[0x22]);
	*width        = *(int*)&(header[0x12]);
	*height       = *(int*)&(header[0x16]);
	// Some BMP files are misformatted, then guess missing information
	if (imageSize == 0) imageSize = *width * *height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0) dataPos = 54;	// The BMP header is done that way

	// Create a buffer
	unsigned char *data = (unsigned char*) calloc(imageSize,1);
	 
	// Read the actual data from the file into the buffer
	if (fread(data,1,imageSize,fp) != imageSize) {
		printf("Cannot read %d bytes from %s\n",imageSize,fn);
		return NULL;
	}
	 
	//Everything is in memory now, the file can be closed
	if (fclose(fp) != 0) {
		printf("Cannot close %s\n",fn);
		return NULL;
	}

	printf("loadBMP(): dataPos=%d, imageSize=%d, width=%d, height=%d\n",
		dataPos, imageSize, *width, *height);
	return data;
}

static int loadGLTextures() {
	int i, width, height, hasAlpha;
#ifdef BMP
	char *fn[4] = {	// file names of textures
		"/home/klm/c/bmp/SolarBackground.bmp",
		"/home/klm/c/bmp/Sun.bmp",
		"/home/klm/c/bmp/Earth.bmp",
		"/home/klm/c/bmp/Moon.bmp"
	};
#else
	char *fn[4] = {	// file names of textures
		"/home/klm/c/bmp/SolarBackground.png",
		"/home/klm/c/bmp/Sun.png",
		"/home/klm/c/bmp/Earth.png",
		"/home/klm/c/bmp/Moon.png"
	};
#endif
	unsigned char *data;

	glGenTextures(4, &texture[0]);
	for (i=0; i<4; ++i) {
		//if ((data = loadBMP(fn[i],&width,&height)) == NULL) endGtkGL();
		if ((data = loadPng(fn[i],&width,&height,&hasAlpha)) == NULL) endGtkGL();
		printf("texture[%d]=%d, width=%d, height=%d, hasAlpha=%d, fn[%d]=%s\n",
			i,texture[i],width,height,hasAlpha,i,fn[i]);
		glBindTexture(GL_TEXTURE_2D, texture[i]);
		GLERR()
#ifdef BMP
		glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
#else
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		GLERR()
		glTexImage2D(GL_TEXTURE_2D, 0, hasAlpha ? 4 : 3, width,
			height, 0, hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
		GLERR()
#endif
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		GLERR()
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		GLERR()
		free(data);
	}
	//glBindTexture(GL_TEXTURE_2D, texture[0]);	// Background is default
	return TRUE;
}

static int initGL (GLvoid) {					// All Setup For OpenGL Goes Here
	if ( textureFlag && !loadGLTextures() ) return FALSE;	// If Loading The Textures Failed
	glShadeModel(GL_SMOOTH);				// Enable Smooth Shading
	glClearColor(0.03f, 0.03, 0.03f, 0.0f);			// Background
	glEnable(GL_COLOR_MATERIAL);
	glClearDepth(1.0f);					// Depth Buffer Setup
	glClearStencil(0);					// Clear The Stencil Buffer To 0
	glEnable(GL_DEPTH_TEST);				// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);					// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	glEnable(GL_TEXTURE_2D);							// Enable 2D Texture Mapping

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmb);		// Set The Ambient Lighting For Light0
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDif);		// Set The Diffuse Lighting For Light0
	glLightfv(GL_LIGHT0, GL_POSITION, LightPos);		// Set The Position For Light0

	glEnable(GL_LIGHT0);					// Enable Light 0
	glEnable(GL_LIGHTING);					// Enable Lighting

	e = gluNewQuadric();					// Create A New Quadric
	m = gluNewQuadric();
	b = gluNewQuadric();
	s = gluNewQuadric();
	gluQuadricNormals(e, GL_SMOOTH);
	gluQuadricTexture(e, GL_TRUE);						
	gluQuadricNormals(m, GL_SMOOTH);					
	gluQuadricTexture(m, GL_TRUE);	
	gluQuadricNormals(b, GL_SMOOTH);					
	gluQuadricTexture(b, GL_TRUE);	
	gluQuadricNormals(s, GL_SMOOTH);					
	gluQuadricTexture(s, GL_TRUE);	

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);	// Set Up Sphere Mapping
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);	// Set Up Sphere Mapping

	return TRUE;						// Initialization Went OK
}

static void drawBackground() {					// Draws the Floor
	glColor3f(0.1f,0.1f,0.0f);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	GLERR()
	gluSphere(b, 5.0f, 640, 320);
	//gluSphere(b, 3.0f, 640, 320);
}

static void drawSun() {
	glColor3f(0.9f,0.9f,0.0f);
	float emission[] = {1.0, 1.0, 1.0, 1.0};
	glMaterialfv(GL_FRONT, GL_EMISSION, emission); 
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	GLERR()
	gluSphere(s, 3.5f, 320, 160);				// 20 times bigger in ratio to earth and moon
	emission[0] = 0.0;
	emission[1] = 0.0;
	emission[2] = 0.0;
	glMaterialfv(GL_FRONT, GL_EMISSION, emission); 
}

static void drawEarth() {					// Draw Our Ball
	glColor3f(0.0f, 0.3f, 1.0f);				// Set Color To White
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	GLERR()
	gluSphere(e, 1.2756f, 64, 32);				// Real size in ratio to the moon (20 times smaller in ratio to the sun)
}

static void drawMoon() {
	glColor3f(0.2f, 0.0f, 0.0f);				// Set Color To White
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	GLERR()
	gluSphere(m, 0.3476f, 32, 16);				// Real size in ratio to the earth (20 times smaller in ratio to the sun)
}

int drawGLScene(GLvoid) {					// Draw Everything
	// Clear Screen, Depth Buffer & Stencil Buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gluPerspective( 45.0, 1.0, 0.01, 1000.0);
	GLERR()

	glLoadIdentity();					// Reset The Modelview Matrix
	//glMultMatrixf(Transform.M);
	glTranslatef(0.0f, 0.0f, zoom);			// Zoom And Raise Camera Above The Floor (Up 0.6 Units)

	drawBackground();

	//glColor3f(0.1f, 0.0f, 0.7f);
	//glColor3fv(colv);
	//glRectf(-2.0, -2.0, 2.0, 2.0);
	//GLERR()

	glPushMatrix();
		glMultMatrixf(Transform.M);
		glScalef(1.0f, -1.0f, 1.0f);			// Mirror Y Axis
		glTranslatef(0.0f, height, 0.0f);		// Position The Object
		glRotatef(xrotS, 1.0f, 0.0f, 0.0f);
		glRotatef((yrotS*365.25/25), 0.0f, 1.0f, 0.0f);
		float m[16], x, y, z, x1, y1, z1;
		x = 0.0;
		y = 0.0;
		z = 0.0;

		glGetFloatv(GL_MODELVIEW_MATRIX, &m[0]);
		x1 = m[0]*x + m[4]*y + m[8]*z + m[12]*1.0;
		y1 = m[1]*x + m[5]*y + m[9]*z + m[13]*1.0;
		z1 = m[2]*x + m[6]*y + m[10]*z + m[14]*1.0;
		float lightPos1[4];
		lightPos1[0] = x1;
		lightPos1[1] = y1;
		lightPos1[2] = z1;
		lightPos1[3] = 1.0;

		glPushMatrix();
			glLoadIdentity();
			glLightfv(GL_LIGHT0, GL_POSITION, lightPos1);
		glPopMatrix();

		drawSun();
	glPopMatrix();

	glPushMatrix();						// Push The Matrix Onto The Stack
		glMultMatrixf(Transform.M);
		glScalef(1.0f, -1.0f, 1.0f);			// Mirror Y Axis
		glRotatef(yrotS, 0.0f, 1.0f, 0.0f);
		glTranslatef(10.0f, height, 0.0f);		// Position The Object
		glRotatef(23.44, 0.0f, 0.0f, 1.0f);
		glRotatef(xrot, 1.0f, 0.0f, 0.0f);		// Rotate Local Coordinate System On X Axis
		glRotatef(yrotS*365.25, 0.0f, 1.0f, 0.0f);	// in reality: 10 times faster
		glRotatef(90,1.0f,0.0f,0.0f);
		drawEarth();
	glPopMatrix();

	glPushMatrix();		
		glMultMatrixf(Transform.M);		
		glRotatef(yrotS, 0.0f, 1.0f, 0.0f);		// Rotation surround the sun
		glTranslatef(10.0f, height+0.7, 0.0f);		// Translation to the earth
		glRotatef(yrotS*365.25/27.3, 0.0f, 1.0f, 0.0f );// Rotation (real ratio to the earth)
		glTranslatef(2.0f,0.0f,0.0f);			// Translation to his orbit
		glRotatef(6.68, 0.0f, 0.0f, 1.0f);
		glRotatef(xrotM, 1.0f, 0.0f, 0.0f);		// Rotation on his own X Axis
		glRotatef(yrotS*365.25/27.3, 0.0f, 1.0f, 0.0f);	// Rotation on his own Y Axis
		drawMoon();										
	glPopMatrix();

	return TRUE;
}


static void display(GtkWidget *drawing_area) {
	GdkGLContext *gl_context = gtk_widget_get_gl_context(drawing_area);
	GdkGLDrawable *gl_drawable = gtk_widget_get_gl_drawable(drawing_area);

	/* invalidate drawing area, marking it "dirty" and to be redrawn when
		main loop signals expose-events, which it does as needed when it
		returns */
	GtkAllocation alloc;
	gtk_widget_get_allocation(drawing_area, &alloc);
	gdk_window_invalidate_rect(gtk_widget_get_root_window(drawing_area), &alloc, FALSE);

	/* Delimits the begining of the OpenGL execution. */
	if ( !gdk_gl_drawable_gl_begin(gl_drawable, gl_context) )
		g_assert_not_reached();

	drawGLScene();

	//glXSwapBuffers(auxXDisplay(), auxXWindow());
	/* swap buffer if we're using double-buffering */
	if (gdk_gl_drawable_is_double_buffered(gl_drawable))	 
		 gdk_gl_drawable_swap_buffers(gl_drawable);
	else
		 /* All programs should call glFlush whenever 
		    they count on having all of their previously 
		    issued commands completed. */
		glFlush();


	/* Delimits the end of the OpenGL execution. */
	gdk_gl_drawable_gl_end(gl_drawable);
}

static void spinDisplay(GtkWidget *drawing_area) {
	if (!rotat) return;
	spin = spin + 2.0;
	if (spin > 360.0) spin -= 360.0;

	xrot += xrotspeed;									
	yrot += yrotspeed;									
	xrotM += xrotMspeed;									
	yrotM += yrotMspeed;
	xrotS += xrotSspeed;
	yrotS += yrotSspeed;
	if ( yrotS > 360.0 ) yrotS -= 360.0;
	if ( xrotS > 360.0 ) xrotS -= 360.0;
	if ( yrotM > 360.0 ) yrotM -= 360.0;
	if ( xrotM > 360.0 ) xrotM -= 360.0;
	if ( yrot > 360.0 ) yrot -= 360.0;
	if ( xrot > 360.0 ) xrot -= 360.0;

	display(drawing_area);
}


static void myReshape(GLsizei w, GLsizei h) {
	if (h == 0) h = 1;	// Prevent A Divide By Zero By
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ArcBallSetBounds(&ArcBall,w,h);
#ifdef DDT
	if (w <= h)
		 glOrtho (-50.0, 50.0, -50.0*(GLfloat)h/(GLfloat)w,
				50.0*(GLfloat)h/(GLfloat)w, -1.0, 1.0);
	else
		 glOrtho (-50.0*(GLfloat)w/(GLfloat)h,
				50.0*(GLfloat)w/(GLfloat)h, -50.0, 50.0, -1.0, 1.0);

	// Calculate The Aspect Ratio Of The Window
#endif
	gluPerspective(45.0f, (GLfloat)w/ (GLfloat)h, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();		// Reset The Modelview Matrix
}


static gboolean delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
	g_print("Received delete event\n");
	return FALSE;
}

static gboolean expose_cb(GtkWidget *drawing_area, GdkEventExpose *event, gpointer userData) {
	g_print("expose_cb called\n");

	display(drawing_area);

	return TRUE;
}

static gboolean btpress_cb(GtkWidget *drawing_area, GdkEvent *event, gpointer userData) {
	g_print("btpress_cb called\n");
	printf("x=%f, y=%f, state=%u, button=%u\n",event->button.x, event->button.y, event->button.state, event->button.button);
	if (event->button.button == 3) {	// right click
		Matrix3fSetIdentity(&LastRot);	// Reset Rotation
		Matrix3fSetIdentity(&ThisRot);	// Reset Rotation
		Matrix4fSetRotationFromMatrix3f(&Transform, &ThisRot); // Reset Rotation
	} else {
		ArcBallClick(&ArcBall, event->button.x, event->button.y);
	}
	colv[0] = 1.0 - colv[0];	// change color
	colv[1] = 1.0 - colv[0];
	colv[2] = 1.0 - colv[0];
	colv[3] = 1.0 - colv[0];
	return TRUE;
}

static gboolean btrelease_cb(GtkWidget *drawing_area, GdkEvent *event, gpointer userData) {
	Quat4fT ThisQuat;
	g_print("btrelease_cb called\n");
	printf("x=%f, y=%f, state=%u, button=%u\n",event->button.x, event->button.y, event->button.state, event->button.button);
	ArcBallDrag(&ArcBall,event->button.x, event->button.y, &ThisQuat);// Update End Vector And Get Rotation As Quaternion
	Matrix3fSetRotationFromQuat4f(&ThisRot, &ThisQuat);	// Convert Quaternion Into Matrix3fT
	Matrix3fMulMatrix3f(&ThisRot, &LastRot);		// Accumulate Last Rotation Into This One
	Matrix4fSetRotationFromMatrix3f(&Transform, &ThisRot);	// Set Our Final Transform's Rotation From This One
	return TRUE;
}

static gboolean keypress_cb(GtkWidget *drawing_area, GdkEvent *event, gpointer userData) {
	g_print("keypress_cb called\n");
	printf("keyval=%X\n",event->key.keyval);
	if (event->key.keyval == GDK_KEY_Escape) endGtkGL();
	else if (event->key.keyval == GDK_KEY_space) rotat = 1 - rotat;
	else if (event->key.keyval == GDK_KEY_Right) {
		yrotSspeed += 0.01f;	// Right Arrow Pressed (Increase yrotspeed) (against clockwise)
		printf("yrotSspeed=%f\n",yrotSspeed); 
	} else if (event->key.keyval == GDK_KEY_Left) {
		yrotSspeed -= 0.01f;	// Left Arrow Pressed (Decrease yrotspeed) (against clockwise)
		printf("yrotSspeed=%f\n",yrotSspeed); 
	} else if (event->key.keyval == GDK_KEY_Down)	xrotSspeed += 0.01f;	// Down Arrow Pressed (Increase xrotspeed)
	else if (event->key.keyval == GDK_KEY_Up)	xrotSspeed -= 0.01f;	// Up Arrow Pressed (Decrease xrotspeed)
	else if (event->key.keyval == GDK_KEY_R) {	// Reset rotation values
		yrotMspeed = 0.0f; xrotMspeed = 0.0f;
		xrotspeed = 0.0f; yrotspeed = 0.0f;
		xrotSspeed = 0.0f; yrotSspeed = 0.0f;
	}

	/* * *
	// Control moon:
	else if (event->key.keyval == GDK_KEY_D)	yrotMspeed += 0.1f;	// Right Arrow Pressed (Increase yrotspeed)
	else if (event->key.keyval == GDK_KEY_A)	yrotMspeed -= 0.1f;	// Left Arrow Pressed (Decrease yrotspeed)
	else if (event->key.keyval == GDK_KEY_W)	xrotMspeed += 0.1f;	// Down Arrow Pressed (Increase xrotspeed)
	else if (event->key.keyval == GDK_KEY_S)	xrotMspeed -= 0.1f;	// Up Arrow Pressed (Decrease xrotspeed)

	// Control sun:
	else if (event->key.keyval == GDK_KEY_H)	yrotSspeed += 0.1f;	// Right Arrow Pressed (Increase yrotspeed)
	else if (event->key.keyval == GDK_KEY_F)	yrotSspeed -= 0.1f;	// Left Arrow Pressed (Decrease yrotspeed)
	else if (event->key.keyval == GDK_KEY_T)	xrotSspeed += 0.1f;	// Down Arrow Pressed (Increase xrotspeed)
	else if (event->key.keyval == GDK_KEY_G)	xrotSspeed -= 0.1f;	// Up Arrow Pressed (Decrease xrotspeed)
	* * */

	else if (event->key.keyval == GDK_KEY_I)	zoom += 0.2f;	// 'I' Key Pressed ... Zoom In
	else if (event->key.keyval == GDK_KEY_O)	zoom -= 0.2f;	// 'O' Key Pressed ... Zoom Out
	else if (event->key.keyval == GDK_KEY_Page_Up) {
		height +=0.02f;	// Page Up Key Pressed Move Ball Up
		printf("Height=%f\n",height);
	} else if (event->key.keyval == GDK_KEY_Page_Down) {
		height -=0.02f;	// Page Down Key Pressed Move Ball Down
		printf("Height=%f\n",height);
	}

	return TRUE;
}

static gboolean scroll_cb(GtkWidget *drawing_area, GdkEventScroll *scroll, gpointer userData) {
	g_print("scroll_cb called\n");
	printf("direction=%X\n",scroll->direction);
	if (scroll->direction == GDK_SCROLL_UP) zoom += 0.2f;
	else if (scroll->direction == GDK_SCROLL_DOWN) zoom -= 0.2f;
	printf("zoom = %f\n", zoom);
	return TRUE;
}

static gboolean motion_cb(GtkWidget *drawing_area, GdkEvent *event, gpointer userData) {
	if (event->button.state & GDK_BUTTON1_MASK) {
		g_print("\tmotion_cb called\n");
		printf("\tx=%f, y=%f, state=%u, button=%u\n",event->button.x, event->button.y, event->button.state, event->button.button);
		Quat4fT ThisQuat;
		ArcBallDrag(&ArcBall,event->button.x, event->button.y, &ThisQuat);// Update End Vector And Get Rotation As Quaternion
		Matrix3fSetRotationFromQuat4f(&ThisRot, &ThisQuat);     // Convert Quaternion Into Matrix3fT
		Matrix3fMulMatrix3f(&ThisRot, &LastRot);                // Accumulate Last Rotation Into This One
		Matrix4fSetRotationFromMatrix3f(&Transform, &ThisRot);  // Set Our Final Transform's Rotation From This One
		ArcBallPrint(&ArcBall);
		Matrix4Print(&Transform);

		display(drawing_area);
	}
	return TRUE;
}


static gboolean idle_cb(gpointer user_data) {
	//g_print("idle_cb called\n");
	/* update control data/params  in this function if needed */
	GtkWidget *drawing_area = GTK_WIDGET(user_data);
	spinDisplay(drawing_area);
	return TRUE;
}

static gboolean configure_cb(GtkWidget *drawing_area, GdkEventConfigure *event, gpointer user_data) {
	static int GLinitOK = 0;				// want to call initGL() only once
	g_print("configure_cb called\n");
	GdkGLContext *gl_context = gtk_widget_get_gl_context(drawing_area);
	GdkGLDrawable *gl_drawable = gtk_widget_get_gl_drawable(drawing_area);

	/* Delimits the begining of the OpenGL execution. */
	if (!gdk_gl_drawable_gl_begin(gl_drawable, gl_context))
		 g_assert_not_reached();

	// specify the lower left corner of our viewport, as well as width/height of the viewport
	GtkAllocation alloc;
	gtk_widget_get_allocation(drawing_area, &alloc);
	//glViewport(0, 0, alloc.width, alloc.height);
	myReshape(alloc.width, alloc.height);

	const static GLfloat light0_position[] = {1.0, 1.0, 1.0, 0.0};
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glEnable(GL_DEPTH_TEST);	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0); 
	glEnable(GL_COLOR_MATERIAL);

	if (GLinitOK == 0) {
		initGL();
		GLinitOK = 1;
	}

	/* Delimits the end of the OpenGL execution. */
	gdk_gl_drawable_gl_end(gl_drawable);

	return TRUE;
}


int main(int argc, char *argv[]) {
	int c;
	while ((c = getopt(argc,argv,"dt")) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 't':
			textureFlag = 1;
			break;
		default:
			fprintf(stderr,"%s: unknown option flag %c\n",argv[0], c);
			return 1;
		}
	}
	/* must be called once in every gtk program */
	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Sun Earth Moon");

	gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
	GtkWidget *drawing_area = gtk_drawing_area_new();

	/* put drawing area in our window */
	gtk_container_add(GTK_CONTAINER(window), drawing_area);

	/* register window callbacks */
	g_signal_connect_swapped(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(delete_event_cb), NULL);

	gtk_widget_set_events(drawing_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_POINTER_MOTION_MASK
		| GDK_POINTER_MOTION_HINT_MASK | GDK_SCROLL_MASK);

	gtk_gl_init(&argc, &argv);

	/* prepare GL */
	GdkGLConfig *gl_config = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE);
	if (!gl_config) g_assert_not_reached();

	if (!gtk_widget_set_gl_capability(drawing_area, gl_config, NULL, TRUE, GDK_GL_RGBA_TYPE))
		 g_assert_not_reached();

	/* only called once in practice */
	g_signal_connect(drawing_area, "configure-event", G_CALLBACK(configure_cb), NULL);
	/* called every time we need to redraw due to becoming visible or being resized */
	g_signal_connect(drawing_area, "expose-event", G_CALLBACK(expose_cb), NULL);
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(btpress_cb), NULL);
	g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(btrelease_cb), NULL);
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(keypress_cb), NULL);
	g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(scroll_cb), NULL);
	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(motion_cb), NULL);

	const gdouble TIMEOUT_PERIOD = 1000 / 20;
	/* idleCb called every timeoutPeriod */
	g_timeout_add(TIMEOUT_PERIOD, idle_cb, drawing_area);

	gtk_widget_show_all(window);

	gtk_main(); /* our main event loop will roll here */

	return 0;
}

