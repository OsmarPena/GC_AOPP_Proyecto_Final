#include "estructuras.h"
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 

void renderizarArbol(Joint *nodo) {
    if (nodo == NULL)
        return;

    glPushMatrix(); 
        glTranslatef(nodo->pos_local.x + nodo->pos_actual.x, 
                     nodo->pos_local.y + nodo->pos_actual.y, 
                     nodo->pos_local.z + nodo->pos_actual.z);
        
        glRotatef(nodo->rot_local.x + nodo->rot_actual.x, 1, 0, 0);
        glRotatef(nodo->rot_local.y + nodo->rot_actual.y, 0, 1, 0);
        glRotatef(nodo->rot_local.z + nodo->rot_actual.z, 0, 0, 1);
        
        if (nodo->esc_local.x != 0) 
            glScalef(nodo->esc_local.x, nodo->esc_local.y, nodo->esc_local.z);

        if (nodo->drawFunc != NULL) {
            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
            
            glColor3fv(nodo->material.diffuse);
            
            nodo->drawFunc(); 
            
            glDisable(GL_COLOR_MATERIAL);
        }

        for (int i = 0; i < nodo->num_hijos; i++) {
            if (nodo->hijo[i] != NULL) renderizarArbol(nodo->hijo[i]);
        }

    glPopMatrix(); 
}

void dibujarCubo() {
    glutSolidCube(1.0);
}

Joint* crearJointSimple(char* nombre) {
    Joint* j = (Joint*)malloc(sizeof(Joint));
    strcpy(j->nombre, nombre);
    j->num_hijos = 0; j->id = -1; j->textura_id = 0;
    
    j->pos_local = (Vec3){0,0,0};
    j->pos_actual = (Vec3){0,0,0};
    
    j->rot_local = (Vec3){0,0,0};
    j->rot_actual = (Vec3){0,0,0};
    
    j->esc_local = (Vec3){1,1,1}; 
    for(int i=0; i<10; i++) j->hijo[i] = NULL;
    j->drawFunc = dibujarCubo; 
    
    j->material.diffuse[0] = 1.0;
    j->material.diffuse[1] = 0.0;
    j->material.diffuse[2] = 1.0;
    j->material.diffuse[3] = 1.0;
    
    return j;
}

Joint* armarRobot() {
    
    Joint* torso = crearJointSimple("Torso");
    torso->pos_local.y = 0.5; 
    torso->esc_local = (Vec3){0.85, 0.75, 0.5}; 
    torso->material.diffuse[0] = 0.0f;
    torso->material.diffuse[1] = 0.0f;
    torso->material.diffuse[2] = 0.0f;

    Joint* cuello = crearJointSimple("Cuello");
    cuello->pos_local.y = 0.55;
    cuello->esc_local = (Vec3){0.2, 0.2, 0.2};
    cuello->material.diffuse[0] = 1.0f;
    cuello->material.diffuse[1] = 1.0f;
    cuello->material.diffuse[2] = 1.0f;

    Joint* cabeza = crearJointSimple("Cabeza");
    cabeza->pos_local.y = 1.0; 
    cabeza->esc_local = (Vec3){0.7, 0.7, 0.7}; 
    cabeza->material.diffuse[0] = 1.0;
    cabeza->material.diffuse[1] = 1.0;
    cabeza->material.diffuse[2] = 1.0;

    Joint* ojoIzq = crearJointSimple("OjoI");
    ojoIzq->pos_local.z = 0.55; 
    ojoIzq->pos_local.x = -0.2; 
    ojoIzq->pos_local.y = 0.05; 
    ojoIzq->esc_local = (Vec3){0.2, 0.35, 0.1}; 
    ojoIzq->material.diffuse[0] = 0.0f;
    ojoIzq->material.diffuse[1] = 0.0f;
    ojoIzq->material.diffuse[2] = 0.0f; 

    Joint* ojoDer = crearJointSimple("OjoD");
    ojoDer->pos_local.z = 0.55; 
    ojoDer->pos_local.x = 0.2;  
    ojoDer->pos_local.y = 0.05; 
    ojoDer->esc_local = (Vec3){0.2, 0.35, 0.1}; 
    ojoDer->material.diffuse[0] = 0.0f;
    ojoDer->material.diffuse[1] = 0.0f;
    ojoDer->material.diffuse[2] = 0.0f; 

    Joint* brazoI = crearJointSimple("BrazoI");
    brazoI->pos_local.x = -0.65;
    brazoI->pos_local.y = 0.1;
    brazoI->esc_local = (Vec3){0.25, 0.7, 0.3};
    brazoI->material.diffuse[0] = 1.0f;
    brazoI->material.diffuse[1] = 1.0f;
    brazoI->material.diffuse[2] = 1.0f;

    Joint* brazoD = crearJointSimple("BrazoD");
    brazoD->pos_local.x = 0.65;
    brazoD->pos_local.y = 0.1;
    brazoD->esc_local = (Vec3){0.25, 0.7, 0.3}; 
    brazoD->material.diffuse[0] = 1.0f;
    brazoD->material.diffuse[1] = 1.0f;
    brazoD->material.diffuse[2] = 1.0f;

    Joint* piernaI = crearJointSimple("PiernaI");
    piernaI->pos_local.x = -0.25;
    piernaI->pos_local.y = -0.8; 
    piernaI->esc_local = (Vec3){0.25, 0.8, 0.3}; 
    piernaI->material.diffuse[0] = 1.0f;
    piernaI->material.diffuse[1] = 1.0f;
    piernaI->material.diffuse[2] = 1.0f; 

    Joint* piernaD = crearJointSimple("PiernaD");
    piernaD->pos_local.x = 0.25;
    piernaD->pos_local.y = -0.8; 
    piernaD->esc_local = (Vec3){0.25, 0.8, 0.3}; 
    piernaD->material.diffuse[0] = 1.0f;
    piernaD->material.diffuse[1] = 1.0f;
    piernaD->material.diffuse[2] = 1.0f;

    // Ojos hijos de Cabeza
    cabeza->hijo[0] = ojoIzq;
    cabeza->hijo[1] = ojoDer;
    cabeza->num_hijos = 2;

    // Torso padre de todo lo demás
    torso->hijo[0] = cuello;
    torso->hijo[1] = cabeza;
    torso->hijo[2] = piernaI;
    torso->hijo[3] = piernaD;
    torso->hijo[4] = brazoI;
    torso->hijo[5] = brazoD;
    torso->num_hijos = 6;
    
    // IDs para animación
    torso->id = 0;
    cabeza->id = 1;
    piernaI->id = 2;
    piernaD->id = 3;
    brazoI->id = 4;
    brazoD->id = 5;

    return torso; 
}

void dibujarPiso() {
    glDisable(GL_LIGHTING); 
    glDisable(GL_TEXTURE_2D);

    glColor3f(0.28f, 0.55f, 0.25f);
    
    glBegin(GL_QUADS);
        glVertex3f(-20.0, -0.5, -20.0);
        glVertex3f(-20.0, -0.5,  20.0);
        glVertex3f( 20.0, -0.5,  20.0);
        glVertex3f( 20.0, -0.5, -20.0);
    glEnd();
    
    glEnable(GL_LIGHTING); 
}