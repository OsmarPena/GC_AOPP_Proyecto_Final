#include <GL/glut.h>
#include <stdio.h>
#include "estructuras.h"

// Importamos las funciones del otro archivo
extern Joint* armarRobot();
extern void renderizarArbol(Joint *nodo);
extern void dibujarPiso();

// Variable global para el personaje
Joint *personaje;

void init() {
    glClearColor(0.53f, 0.80f, 0.92f, 1.0f);
    glEnable(GL_DEPTH_TEST);    //
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat luz_pos[] = { 10.0f, 10.0f, 10.0f, 0.0f }; 
    glLightfv(GL_LIGHT0, GL_POSITION, luz_pos);

    GLfloat luz_blanca[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_blanca);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz_blanca);

    GLfloat ambiente[] = { 0.4f, 0.4f, 0.4f, 1.0f }; 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambiente);
    
    personaje = armarRobot();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    gluLookAt(0.0, 2.0, 5.0,
              0.0, 1.0, 0.0,
              0.0, 1.0, 0.0);

    dibujarPiso(); 
    if (personaje != NULL) renderizarArbol(personaje);
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // gluPerspective(Angulo, Aspecto, Cercania, Lejania)
    gluPerspective(45.0, aspect, 0.1, 100.0); 

    glMatrixMode(GL_MODELVIEW);
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Proyecto Final");

    init();

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutMainLoop();
    return 0;
}