#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include "estructuras.h"

// Variables globales
int animando = 1;   // Inicia reproduciendo automáticamente (1)
Joint *personaje = NULL;

// Funciones externas
extern Joint* armarRobot();
extern void renderizarArbol(Joint *nodo);
extern void dibujarPiso();
extern void actualizarAnimacion(Joint* robot);
extern void iniciarGuion();
extern float obtenerTiempoActual();
extern void actualizarCamara();

// Cámara dinámica (Controlada por animacion.c)
extern Camara camara_escena; 

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Camara dinámica
    gluLookAt(camara_escena.eye.x, camara_escena.eye.y, camara_escena.eye.z,
              camara_escena.center.x, camara_escena.center.y, camara_escena.center.z,
              0.0, 1.0, 0.0);

    dibujarPiso(); 
    if (personaje != NULL)
        renderizarArbol(personaje);
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 0.1, 100.0); 

    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    if (animando == 1) {
        actualizarAnimacion(personaje);
        glutPostRedisplay();
    }
    glutTimerFunc(42, timer, 0); // ~24 FPS
}

void init() {
    glClearColor(0.53f, 0.80f, 0.92f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE); 

    // Configuración de Luz Básica
    GLfloat luz_pos[] = { 10.0f, 10.0f, 10.0f, 0.0f }; 
    glLightfv(GL_LIGHT0, GL_POSITION, luz_pos);

    GLfloat luz_blanca[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_blanca);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz_blanca);

    GLfloat ambiente[] = { 0.6f, 0.6f, 0.6f, 1.0f }; 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambiente);
    
    personaje = armarRobot();
    iniciarGuion();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(50, 50);
    
    glutCreateWindow("Proyecto Final");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}