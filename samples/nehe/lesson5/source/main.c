#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ppu-types.h>

#include <io/pad.h>
#include <sysutil/sysutil.h>
#include <ps3gl.h>
#include <GL/gl.h>

bool running = 1;
float rtri = 0.0f;
float rquad = 0.0f;

int DrawGLScene(GLvoid)                     // Here's Where We Do All The Drawing
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
    glLoadIdentity();                   // Reset The View
    glTranslatef(-1.5f,0.0f,-6.0f);             // Move Left And Into The Screen
 
    glRotatef(rtri,0.0f,1.0f,0.0f);             // Rotate The Pyramid On It's Y Axis
 
    glBegin(GL_TRIANGLES);                  // Start Drawing The Pyramid
		glColor3f(1.0f,0.0f,0.0f);          // Red
		glVertex3f( 0.0f, 1.0f, 0.0f);          // Top Of Triangle (Front)
		glColor3f(0.0f,1.0f,0.0f);          // Green
		glVertex3f(-1.0f,-1.0f, 1.0f);          // Left Of Triangle (Front)
		glColor3f(0.0f,0.0f,1.0f);          // Blue
		glVertex3f( 1.0f,-1.0f, 1.0f);          // Right Of Triangle (Front)

		glColor3f(1.0f,0.0f,0.0f);          // Red
		glVertex3f( 0.0f, 1.0f, 0.0f);          // Top Of Triangle (Right)
		glColor3f(0.0f,0.0f,1.0f);          // Blue
		glVertex3f( 1.0f,-1.0f, 1.0f);          // Left Of Triangle (Right)
		glColor3f(0.0f,1.0f,0.0f);          // Green
		glVertex3f( 1.0f,-1.0f, -1.0f);         // Right Of Triangle (Right)

		glColor3f(1.0f,0.0f,0.0f);          // Red
		glVertex3f( 0.0f, 1.0f, 0.0f);          // Top Of Triangle (Back)
		glColor3f(0.0f,1.0f,0.0f);          // Green
		glVertex3f( 1.0f,-1.0f, -1.0f);         // Left Of Triangle (Back)
		glColor3f(0.0f,0.0f,1.0f);          // Blue
		glVertex3f(-1.0f,-1.0f, -1.0f);         // Right Of Triangle (Back)

		glColor3f(1.0f,0.0f,0.0f);          // Red
		glVertex3f( 0.0f, 1.0f, 0.0f);          // Top Of Triangle (Left)
		glColor3f(0.0f,0.0f,1.0f);          // Blue
		glVertex3f(-1.0f,-1.0f,-1.0f);          // Left Of Triangle (Left)
		glColor3f(0.0f,1.0f,0.0f);          // Green
		glVertex3f(-1.0f,-1.0f, 1.0f);          // Right Of Triangle (Left)
	glEnd();                        // Done Drawing The Pyramid

	glLoadIdentity();
	glTranslatef(1.5f,0.0f,-7.0f);              // Move Right And Into The Screen
	
	glRotatef(rquad,1.0f,1.0f,1.0f);            // Rotate The Cube On X, Y & Z
	
	glBegin(GL_QUADS);                  // Start Drawing The Cube
		glColor3f(0.0f,1.0f,0.0f);          // Set The Color To Green
		glVertex3f( 1.0f, 1.0f,-1.0f);          // Top Right Of The Quad (Top)
		glVertex3f(-1.0f, 1.0f,-1.0f);          // Top Left Of The Quad (Top)
		glVertex3f(-1.0f, 1.0f, 1.0f);          // Bottom Left Of The Quad (Top)
		glVertex3f( 1.0f, 1.0f, 1.0f);          // Bottom Right Of The Quad (Top)

		glColor3f(1.0f,0.5f,0.0f);          // Set The Color To Orange
		glVertex3f( 1.0f,-1.0f, 1.0f);          // Top Right Of The Quad (Bottom)
		glVertex3f(-1.0f,-1.0f, 1.0f);          // Top Left Of The Quad (Bottom)
		glVertex3f(-1.0f,-1.0f,-1.0f);          // Bottom Left Of The Quad (Bottom)
		glVertex3f( 1.0f,-1.0f,-1.0f);          // Bottom Right Of The Quad (Bottom)

		glColor3f(1.0f,0.0f,0.0f);          // Set The Color To Red
		glVertex3f( 1.0f, 1.0f, 1.0f);          // Top Right Of The Quad (Front)
		glVertex3f(-1.0f, 1.0f, 1.0f);          // Top Left Of The Quad (Front)
		glVertex3f(-1.0f,-1.0f, 1.0f);          // Bottom Left Of The Quad (Front)
		glVertex3f( 1.0f,-1.0f, 1.0f);          // Bottom Right Of The Quad (Front)

		glColor3f(1.0f,1.0f,0.0f);          // Set The Color To Yellow
		glVertex3f( 1.0f,-1.0f,-1.0f);          // Bottom Left Of The Quad (Back)
		glVertex3f(-1.0f,-1.0f,-1.0f);          // Bottom Right Of The Quad (Back)
		glVertex3f(-1.0f, 1.0f,-1.0f);          // Top Right Of The Quad (Back)
		glVertex3f( 1.0f, 1.0f,-1.0f);          // Top Left Of The Quad (Back)

		glColor3f(0.0f,0.0f,1.0f);          // Set The Color To Blue
		glVertex3f(-1.0f, 1.0f, 1.0f);          // Top Right Of The Quad (Left)
		glVertex3f(-1.0f, 1.0f,-1.0f);          // Top Left Of The Quad (Left)
		glVertex3f(-1.0f,-1.0f,-1.0f);          // Bottom Left Of The Quad (Left)
		glVertex3f(-1.0f,-1.0f, 1.0f);          // Bottom Right Of The Quad (Left)

        glColor3f(1.0f,0.0f,1.0f);          // Set The Color To Violet
        glVertex3f( 1.0f, 1.0f,-1.0f);          // Top Right Of The Quad (Right)
        glVertex3f( 1.0f, 1.0f, 1.0f);          // Top Left Of The Quad (Right)
        glVertex3f( 1.0f,-1.0f, 1.0f);          // Bottom Left Of The Quad (Right)
        glVertex3f( 1.0f,-1.0f,-1.0f);          // Bottom Right Of The Quad (Right)
    glEnd();                        // Done Drawing The Quad
 
    rtri+=0.2f;                     // Increase The Rotation Variable For The Triangle
    rquad-=0.15f;                       // Decrease The Rotation Variable For The Quad 

	return GL_TRUE;
}

#include <vectormath/c/vectormath_aos.h>
GLvoid ReSizeGLScene(GLsizei width, GLsizei height)             // Resize And Initialize The GL Window
{
    if (height==0)                              // Prevent A Divide By Zero By
    {
        height=1;                           // Making Height Equal One
    }
 
    glViewport(0, 0, width, height);                    // Reset The Current Viewport

    glMatrixMode(GL_PROJECTION);                        // Select The Projection Matrix
    glLoadIdentity();                           // Reset The Projection Matrix
 
    VmathMatrix4 m;
    vmathM4MakePerspective(&m, 45.0f, (GLfloat)width/(GLfloat)height, 0.1f, 100.0f);
    glMultMatrixf((const GLfloat *)&m);
 
    glMatrixMode(GL_MODELVIEW);                     // Select The Modelview Matrix
    glLoadIdentity();                           // Reset The Modelview Matrix
}

extern u32 display_width;
extern u32 display_height;
int main(int argc,const char *argv[])
{
	padInfo padinfo;
	padData paddata;

	ps3glInit();
	glShadeModel(GL_SMOOTH);                        // Enables Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);                   // Black Background
	glClearDepth(1.0f);                         // Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);                        // Enables Depth Testing
	glDepthFunc(GL_LEQUAL);                         // The Type Of Depth Test To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);          // Really Nice Perspective Calculations
	ReSizeGLScene(display_width, display_height);

	ioPadInit(7);
	printf("rsxtest started...\n");
	
	while(DrawGLScene()) {
		sysUtilCheckCallback();

		ioPadGetInfo(&padinfo);
		for(int i=0; i < MAX_PADS; i++){
			if(padinfo.status[i]){
				ioPadGetData(i, &paddata);

				if(paddata.BTN_CROSS)
					goto done;
			}

		}
		ps3glSwapBuffers();
	}

done:
    printf("rsxtest done...\n");
    return 0;
}
