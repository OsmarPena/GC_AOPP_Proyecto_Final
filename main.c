#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estructuras.h"

int animando = 1; // La animación empieza al instante
Joint *personaje = NULL;

// Identificadores de ventanas
int ventana_principal;
int menu_subventana;

// Dimensiones
int ancho_ventana = 800;
int alto_ventana = 600;
const int alto_subventana = 80;

// Funciones externas
extern Joint* armarRobot();
extern void renderizarArbol(Joint *nodo);
extern void dibujarPiso();
extern void actualizarAnimacion(Joint* robot);
extern void iniciarGuion();
extern void reiniciarAnimacion();
extern float obtenerTiempoActual();
extern void actualizarCamara();
extern Camara camara_escena;

void mostrarTexto(float x, float y, const char *string, void *font) {
    glRasterPos2f(x, y);
    int len = (int)strlen(string);
    for (int i = 0; i < len; i++) {
        glutBitmapCharacter(font, string[i]);
    }
}

void agregarBoton(float x, float y, float ancho, float alto, float r, float g, float b, int tipo) {
    // Fondo (Color metálico)
    glColor3f(r, g, b);
    glRectf(x, y, x + ancho, y + alto);

    // Borde del botón
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + ancho, y);
        glVertex2f(x + ancho, y + alto);
        glVertex2f(x, y + alto);
    glEnd();

    // Color de los íconos
    glColor3f(0.0f, 0.0f, 0.0f);

    if (tipo == 1) { // Botón de play
        glBegin(GL_TRIANGLES);
            glVertex2f(x + ancho * 0.4f, y + alto * 0.3f);
            glVertex2f(x + ancho * 0.7f, y + alto * 0.5f);
            glVertex2f(x + ancho * 0.4f, y + alto * 0.7f);
        glEnd();
    } 
    else if (tipo == 2) { // Botón de pausa
        glRectf(x + ancho * 0.35f, y + alto * 0.3f, x + ancho * 0.45f, y + alto * 0.7f);
        glRectf(x + ancho * 0.55f, y + alto * 0.3f, x + ancho * 0.65f, y + alto * 0.7f);
    }
    else if (tipo == 3) {   // Botón de reinicio
        glRectf(x + ancho * 0.3f, y + alto * 0.3f, x + ancho * 0.4f, y + alto * 0.7f);

        glBegin(GL_TRIANGLES);
            glVertex2f(x + ancho * 0.4f, y + alto * 0.5f);
            glVertex2f(x + ancho * 0.7f, y + alto * 0.7f);
            glVertex2f(x + ancho * 0.7f, y + alto * 0.3f);
        glEnd();
    }
}

// Sub-ventana
void displayMenu() {
    glClearColor(0.92f, 0.92f, 0.94f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, ancho_ventana, 0, alto_subventana);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Borde de la sub-ventana
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(0, 0);
        glVertex2f(ancho_ventana, 0);
        glVertex2f(ancho_ventana, alto_subventana);
        glVertex2f(0, alto_subventana);
    glEnd();

    float boton_tam = 50; 
    float espacio = 30;
    float ancho_total = (2 * boton_tam) + espacio; 
    float x_pos = (ancho_ventana - ancho_total) / 2.0f;
    float y_pos = (alto_subventana - boton_tam) / 2.0f;

    // 1. Play/Pausa
    if (animando) 
        // Pausa
        agregarBoton(x_pos, y_pos, boton_tam, boton_tam, 0.8f, 0.55f, 0.35f, 2); 
    else 
        // Play
        agregarBoton(x_pos, y_pos, boton_tam, boton_tam, 0.4f, 0.65f, 0.5f, 1); 
    
    // Reiniciar
    agregarBoton(x_pos + boton_tam + espacio, y_pos, boton_tam, boton_tam, 0.55f, 0.65f, 0.75f, 3); 
    
    // Tiempo en pantalla
    char buffer[20];
    sprintf(buffer, "%.1f s", obtenerTiempoActual());
    
    glColor3f(0.0f, 0.0f, 0.0f);
    mostrarTexto(ancho_ventana - 80, 35, buffer, GLUT_BITMAP_HELVETICA_18);

    glutSwapBuffers();
}

void mouseMenu(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int glY = alto_subventana - y; 

        float boton_tam = 50;
        float espacio = 30;
        float ancho_total = (2 * boton_tam) + espacio; 
        float x_pos = (ancho_ventana - ancho_total) / 2.0f;
        float y_pos = (alto_subventana - boton_tam) / 2.0f;

        // Play/Pausa
        if (x >= x_pos && x <= x_pos + boton_tam && glY >= y_pos && glY <= y_pos + boton_tam) {
            animando = !animando;
        }
        
        // Reiniciar
        float x_posRe = x_pos + boton_tam + espacio;
        if (x >= x_posRe && x <= x_posRe + boton_tam && glY >= y_pos && glY <= y_pos + boton_tam) {
            reiniciarAnimacion();
            animando = 1;
        }
        
        glutPostRedisplay();
    }
}

void display() {
    glutSetWindow(ventana_principal); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    gluLookAt(camara_escena.eye.x, camara_escena.eye.y, camara_escena.eye.z,
              camara_escena.center.x, camara_escena.center.y, camara_escena.center.z,
              0.0, 1.0, 0.0);

    dibujarPiso(); 
    if (personaje != NULL) renderizarArbol(personaje);
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    ancho_ventana = w;
    alto_ventana = h;

    if (h > alto_subventana) {
        glViewport(0, alto_subventana, w, h - alto_subventana);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspecto = (float)w / (float)(h - alto_subventana);
    gluPerspective(45.0, aspecto, 0.1, 100.0); 
    glMatrixMode(GL_MODELVIEW);

    glutSetWindow(menu_subventana);
    glutPositionWindow(0, h - alto_subventana);
    glutReshapeWindow(w, alto_subventana);
    
    glutSetWindow(ventana_principal);
}

void timer(int value) {
    if (animando == 1) {
        actualizarAnimacion(personaje);
        glutSetWindow(ventana_principal);
        glutPostRedisplay();
        glutSetWindow(menu_subventana);
        glutPostRedisplay();
    }
    glutTimerFunc(42, timer, 0); 
}

void init() {
    glClearColor(0.53f, 0.80f, 0.92f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE); 

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
    glutInitWindowSize(ancho_ventana, alto_ventana);
    glutInitWindowPosition(50, 50);

    ventana_principal = glutCreateWindow("Proyecto Final");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    menu_subventana = glutCreateSubWindow(ventana_principal, 0,
        alto_ventana - alto_subventana, ancho_ventana, alto_subventana);
    
    glutDisplayFunc(displayMenu);
    glutMouseFunc(mouseMenu);

    glutMainLoop();
    return 0;
}