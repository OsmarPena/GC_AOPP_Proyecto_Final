#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estructuras.h"

/* COMPILACIÓN
WINDOWS: gcc main.c render.c animacion.c -o proyecto.exe -lfreeglut -lopengl32 -lglu32
LINUX: gcc main.c render.c animacion.c -o proyecto -lGL -lGLU -lglut -lm
*/

int animando = 0;
Joint *personaje = NULL;

extern NodoEscena *escena_raiz;
extern ListaObjetos lista_objetos_activos;
extern Camara camara_escena;

int ventana_principal;
int ventana_controles; 

int ancho_main = 800;
int alto_main = 600;

int ancho_menu = 400;
int alto_menu = 100;

extern void renderizarEscena(NodoEscena *nodo);
extern Joint* armarRobot();
extern void iniciarGuion();
extern void actualizarAnimacion(Joint* robot);
extern void reiniciarAnimacion();
extern float obtenerTiempoActual();

extern void renderizarArbol(Joint *nodo);
extern void dibujarPiso();
extern void dibujarParedes();
extern void dibujarCajas(int f, int c, int ff, int fc);
extern void actualizarCamara();
extern void resetearArbol(Joint* nodo);

void mostrarTexto(float x, float y, const char *string, void *font) {
    glRasterPos2f(x, y);
    int len = (int)strlen(string);
    for (int i = 0; i < len; i++) {
        glutBitmapCharacter(font, string[i]);
    }
}

void agregarBoton(float x, float y, float ancho, float alto, float r, float g, float b, int tipo) {
    glColor3f(r, g, b);
    glRectf(x, y, x + ancho, y + alto);

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + ancho, y);
        glVertex2f(x + ancho, y + alto);
        glVertex2f(x, y + alto);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);

    if (tipo == 1) { 
        glBegin(GL_TRIANGLES);
            glVertex2f(x + ancho * 0.4f, y + alto * 0.3f);
            glVertex2f(x + ancho * 0.7f, y + alto * 0.5f);
            glVertex2f(x + ancho * 0.4f, y + alto * 0.7f);
        glEnd();
    } 
    else if (tipo == 2) { 
        glRectf(x + ancho * 0.35f, y + alto * 0.3f, x + ancho * 0.45f, y + alto * 0.7f);
        glRectf(x + ancho * 0.55f, y + alto * 0.3f, x + ancho * 0.65f, y + alto * 0.7f);
    }
    else if (tipo == 3) {
        glRectf(x + ancho * 0.3f, y + alto * 0.3f, x + ancho * 0.4f, y + alto * 0.7f);
        glBegin(GL_TRIANGLES);
            glVertex2f(x + ancho * 0.4f, y + alto * 0.5f);
            glVertex2f(x + ancho * 0.7f, y + alto * 0.7f);
            glVertex2f(x + ancho * 0.7f, y + alto * 0.3f);
        glEnd();
    }
}

void displayMenu() {
    glutSetWindow(ventana_controles);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, ancho_menu, 0, alto_menu);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float boton_tam = 50; 
    float espacio = 30;
    float ancho_total = (2 * boton_tam) + espacio; 
    float x_pos = (ancho_menu - ancho_total) / 2.0f;
    float y_pos = (alto_menu - boton_tam) / 2.0f;

    if (animando) 
        agregarBoton(x_pos, y_pos, boton_tam, boton_tam, 0.8f, 0.55f, 0.35f, 2); 
    else 
        agregarBoton(x_pos, y_pos, boton_tam, boton_tam, 0.4f, 0.65f, 0.5f, 1); 
    
    agregarBoton(x_pos + boton_tam + espacio, y_pos, boton_tam, boton_tam, 0.55f, 0.65f, 0.75f, 3); 
    
    float x_salir = ancho_menu - boton_tam - 10;
    glColor3f(0.7f, 0.0f, 0.0f); 
    glRectf(x_salir, y_pos, x_salir + boton_tam, y_pos + boton_tam);
    
    glColor3f(0.5f, 0.5f, 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x_salir, y_pos);
        glVertex2f(x_salir + boton_tam, y_pos);
        glVertex2f(x_salir + boton_tam, y_pos + boton_tam);
        glVertex2f(x_salir, y_pos + boton_tam);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
        glVertex2f(x_salir + 10, y_pos + 10);
        glVertex2f(x_salir + boton_tam - 10, y_pos + boton_tam - 10);
        glVertex2f(x_salir + boton_tam - 10, y_pos + 10);
        glVertex2f(x_salir + 10, y_pos + boton_tam - 10);
    glEnd();

    char buffer[20];
    sprintf(buffer, "%.1f s", obtenerTiempoActual());
    glColor3f(0.0f, 0.0f, 0.0f);
    mostrarTexto(20, alto_menu/2 - 5, buffer, GLUT_BITMAP_HELVETICA_18);

    glutSwapBuffers();
}

void mouseMenu(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int glY = alto_menu - y; 

        float boton_tam = 50;
        float espacio = 30;
        float ancho_total = (2 * boton_tam) + espacio; 
        float x_pos = (ancho_menu - ancho_total) / 2.0f;
        float y_pos = (alto_menu - boton_tam) / 2.0f;

        if (x >= x_pos && x <= x_pos + boton_tam && glY >= y_pos && glY <= y_pos + boton_tam) {
            animando = !animando;
        }
        
        float x_posRe = x_pos + boton_tam + espacio;
        if (x >= x_posRe && x <= x_posRe + boton_tam && glY >= y_pos && glY <= y_pos + boton_tam) {
            reiniciarAnimacion();
            animando = 1;
        }

        float x_salir = ancho_menu - boton_tam - 10;
        if (x >= x_salir && x <= x_salir + boton_tam && glY >= y_pos && glY <= y_pos + boton_tam) {
            exit(0);
        }
        
        glutPostRedisplay();
    }
}

void displayMain() {
    glutSetWindow(ventana_principal); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    gluLookAt(camara_escena.eye.x, camara_escena.eye.y, camara_escena.eye.z,
              camara_escena.center.x, camara_escena.center.y, camara_escena.center.z,
              camara_escena.up.x, camara_escena.up.y, camara_escena.up.z);

    if (escena_raiz != NULL) {
        renderizarEscena(escena_raiz);
    }
    
    glutSwapBuffers();
}

void reshapeMain(int w, int h) {
    ancho_main = w;
    alto_main = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 0.1, 500.0); 
    glMatrixMode(GL_MODELVIEW);
}

void reshapeMenu(int w, int h) {
    ancho_menu = w;
    alto_menu = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    if (animando == 1) {
        actualizarAnimacion(personaje);
        
        glutSetWindow(ventana_principal);
        glutPostRedisplay();
        
        glutSetWindow(ventana_controles);
        glutPostRedisplay();
    }
    glutTimerFunc(42, timer, 0); 
}

void initEscena() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);

    GLfloat luz_pos[] = { 0.0f, 0.0f, 20.0f, 1.0f }; 
    glLightfv(GL_LIGHT0, GL_POSITION, luz_pos);

    GLfloat luz_blanca[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_blanca);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz_blanca);

    GLfloat ambiente[] = { 0.4f, 0.4f, 0.4f, 1.0f }; 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambiente);
    
    personaje = armarRobot();
    iniciarGuion();

    NodoListaObjeto *aux = lista_objetos_activos.cabeza;
    while(aux != NULL) {
        if (aux->objeto->tipo == 2) {
            aux->objeto->dato = (void*)personaje;
        }
        aux = aux->sgt;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
    // Ventana 1: Escena 3D
    glutInitWindowSize(ancho_main, alto_main);
    glutInitWindowPosition(50, 50);
    ventana_principal = glutCreateWindow("GC AOPP Proyecto Final");
    initEscena();
    glutDisplayFunc(displayMain);
    glutReshapeFunc(reshapeMain);

    // Ventana 2: Controles (Separada)
    glutInitWindowSize(ancho_menu, alto_menu);
    glutInitWindowPosition(50, alto_main + 100); 
    ventana_controles = glutCreateWindow("Sub-ventana");
    glutDisplayFunc(displayMenu);
    glutReshapeFunc(reshapeMenu);
    glutMouseFunc(mouseMenu);

    glutTimerFunc(0, timer, 0);
    glutMainLoop();
    return 0;
}