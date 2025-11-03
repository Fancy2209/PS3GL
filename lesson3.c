// NeHe Lesson 5

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ppu-types.h>

#include <io/pad.h>
#include <sysutil/sysutil.h>
#include <ps3gl.h>
#include <GL/gl.h>

bool running = 1;

void drawFrame(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear The Screen And The Depth Buffer
    glLoadIdentity();                       // Reset The View
	
	glTranslatef(-1.5f,0.0f,-6.0f);                 // Move Left 1.5 Units And Into The Screen 6.0
	
	glBegin(GL_TRIANGLES);                      // Drawing Using Triangles
		glColor3f(1.0f, 0.0f,0.0f);
		glVertex3f( 0.0f, 1.0f, 0.0f);              // Top
		glColor3f(0.0f, 1.0f,0.0f);
		glVertex3f(-1.0f,-1.0f, 0.0f);              // Bottom Left
		glColor3f(0.0f, 0.0f,1.0f);
		glVertex3f( 1.0f,-1.0f, 0.0f);              // Bottom Right
    glEnd();                            // Finished Drawing The Triangle

	glTranslatef(3.0f,0.0f,0.0f);                   // Move Right 3 Units

	glColor3f(0.5f, 0.5f,1.0f);
    glBegin(GL_QUADS);                      // Draw A Quad
        glVertex3f(-1.0f, 1.0f, 0.0f);              // Top Left
        glVertex3f( 1.0f, 1.0f, 0.0f);              // Top Right
        glVertex3f( 1.0f,-1.0f, 0.0f);              // Bottom Right
        glVertex3f(-1.0f,-1.0f, 0.0f);              // Bottom Left
    glEnd();                            // Done Drawing The Quad
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
	
	while(running) {
		sysUtilCheckCallback();

		ioPadGetInfo(&padinfo);
		for(int i=0; i < MAX_PADS; i++){
			if(padinfo.status[i]){
				ioPadGetData(i, &paddata);

				if(paddata.BTN_CROSS)
					goto done;
			}

		}
		drawFrame();

		ps3glSwapBuffers();
	}

done:
    printf("rsxtest done...\n");
    return 0;
}
