#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estructuras.h"

/* COMPILACIÓN
WINDOWS: gcc main.c render.c animacion.c -o proyecto.exe -lfreeglut -lopengl32 -lglu32
LINUX: gcc main.c render.c animacion.c -o proyecto -lGL -lGLU -lglut -lm
*/

int animando = 0; // La animación no empieza al instante
Joint *personaje = NULL;

extern NodoEscena *escena_raiz; // Árbol principal
extern ListaObjetos lista_objetos_activos;
extern Camara camara_escena;

// Identificadores de ventanas
int ventana_principal;
int menu_subventana;

// Dimensiones
int ancho_ventana = 800;
int alto_ventana = 600;
const int alto_subventana = 80;

extern void renderizarEscena(NodoEscena *nodo);
extern Joint* armarRobot();
extern void iniciarGuion();
extern void actualizarAnimacion(Joint* robot);
extern void reiniciarAnimacion();
extern float obtenerTiempoActual();

// Funciones externas
extern void renderizarArbol(Joint *nodo);
extern void dibujarPiso();
extern void dibujarParedes();
extern void dibujarCajas(int f, int c, int ff, int fc);
extern void actualizarCamara();

void mostrarTexto(float x, float y, const char *string, void *font) {
    glRasterPos2f(x, y);
    int len = (int)strlen(string);
    for (int i = 0; i < len; i++) {
        glutBitmapCharacter(font, string[i]);
    }
}

void agregarBoton(float x, float y, float ancho, float alto, float r, float g, float b, int tipo) {
    // Fondo
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

    // Color de los í­conos
    glColor3f(0.0f, 0.0f, 0.0f);

    if (tipo == 1) { // Botó³n de play
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
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
    
    float x_salir = x_pos + 2 * (boton_tam + espacio);

    glColor3f(0.7f, 0.0f, 0.0f); 
    glRectf(x_salir, y_pos, x_salir + boton_tam, y_pos + boton_tam);
    
    // Borde
    glColor3f(0.5f, 0.5f, 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x_salir, y_pos);
        glVertex2f(x_salir + boton_tam, y_pos);
        glVertex2f(x_salir + boton_tam, y_pos + boton_tam);
        glVertex2f(x_salir, y_pos + boton_tam);
    glEnd();

    // X de Salir
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
        glVertex2f(x_salir + 10, y_pos + 10);
        glVertex2f(x_salir + boton_tam - 10, y_pos + boton_tam - 10);
        
        glVertex2f(x_salir + boton_tam - 10, y_pos + 10);
        glVertex2f(x_salir + 10, y_pos + boton_tam - 10);
    glEnd();

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

        // Salir
        float x_salir = x_pos + 2 * (boton_tam + espacio);
        if (x >= x_salir && x <= x_salir + boton_tam && glY >= y_pos && glY <= y_pos + boton_tam) {
            exit(0);
        }
        
        glutPostRedisplay();
    }
}

void display() {
    glutSetWindow(ventana_principal); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Cámara dinámica controlada por la cola de acciones
    gluLookAt(camara_escena.eye.x, camara_escena.eye.y, camara_escena.eye.z,
              camara_escena.center.x, camara_escena.center.y, camara_escena.center.z,
              camara_escena.up.x, camara_escena.up.y, camara_escena.up.z);

    // Esto recorre recursivamente el árbol de escenas (nodos -> hijos)
    if (escena_raiz != NULL) {
        renderizarEscena(escena_raiz);
    }
    
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
    gluPerspective(45.0, aspecto, 0.1, 500.0); 
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

    /* Busca el nodo tipo 2 (Personaje) en la lista de activos
       para asignarle el puntero 'personaje' que se acaba de crear,
       así el árbol de escena sabe qué dibujar. */
    NodoListaObjeto *aux = lista_objetos_activos.cabeza;
    while(aux != NULL) {
        if (aux->objeto->tipo == 2) {
            aux->objeto->dato = (void*)personaje;
            printf("MAIN: Robot vinculado al Nodo de Escena ID=%d\n", aux->objeto->id);
        }
        aux = aux->sgt;
    }
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