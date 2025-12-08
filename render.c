#include "estructuras.h"
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <math.h>

#define PI 3.14159265

static PilaRender pila_global;

Vec3 sumarVec3(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

// Rota un punto p alrededor del origen (0,0,0) según los ángulos euler (ax, ay, az)
Vec3 rotarPunto(Vec3 p, Vec3 angulos) {
    float radX = angulos.x * PI / 180.0f;
    float radY = angulos.y * PI / 180.0f;
    float radZ = angulos.z * PI / 180.0f;
    
    Vec3 r = p;
    float tempY, tempZ, tempX;

    // Rotación X
    tempY = r.y * cos(radX) - r.z * sin(radX);
    tempZ = r.y * sin(radX) + r.z * cos(radX);
    r.y = tempY; r.z = tempZ;

    // Rotación Y
    tempX = r.x * cos(radY) + r.z * sin(radY);
    tempZ = -r.x * sin(radY) + r.z * cos(radY);
    r.x = tempX; r.z = tempZ;

    // Rotación Z
    tempX = r.x * cos(radZ) - r.y * sin(radZ);
    tempY = r.x * sin(radZ) + r.y * cos(radZ);
    r.x = tempX; r.y = tempY;

    return r;
}

void initPila() {
    pila_global.tope = NULL;
    pila_global.cont = 0;
}

void pushComando(int tipo, Vec3 pos, Vec3 rot, Vec3 esc, Material mat) {
    NodoPila* nuevo = (NodoPila*)malloc(sizeof(NodoPila));
    
    nuevo->cmd.tipo = tipo; // 0 = Cubo, 1 = Extremidad
    nuevo->cmd.pos_global = pos;
    nuevo->cmd.rot_global = rot;
    nuevo->cmd.esc_global = esc;
    nuevo->cmd.material = mat;
    
    nuevo->sgt = pila_global.tope;
    pila_global.tope = nuevo;
    pila_global.cont++;
}

NodoPila* popComando() {
    if (pila_global.tope == NULL) return NULL;
    NodoPila* aux = pila_global.tope;
    pila_global.tope = aux->sgt;
    pila_global.cont--;
    return aux;
}

void limpiarPila() {
    while(pila_global.tope != NULL) {
        NodoPila* n = popComando();
        free(n);
    }
}

void dibujarCubo() {
    glutSolidCube(1.0);
}

// Esto hace que el (0,0,0) sea la parte superior del objeto
void dibujarExtremidad() {
    glPushMatrix();
        glTranslatef(0.0, -0.5, 0.0);
        glutSolidCube(1.0);
    glPopMatrix();
}

// Recorre el árbol y calcula dónde debe ir cada pieza
void addComandos(Joint *nodo, Vec3 padrePos, Vec3 padreRot, Vec3 padreEsc) {
    if (nodo == NULL)
        return;

    // Calculo de transformación local total
    Vec3 offset_local = sumarVec3(nodo->pos_local, nodo->pos_actual);
    Vec3 rot_local_total = sumarVec3(nodo->rot_local, nodo->rot_actual);

    // Antes de rotar, debemos aplicar la escala del padre al offset.
    Vec3 offset_escalado;
    offset_escalado.x = offset_local.x * padreEsc.x;
    offset_escalado.y = offset_local.y * padreEsc.y;
    offset_escalado.z = offset_local.z * padreEsc.z;

    // Calculo de posición global
    Vec3 offsetRotado = rotarPunto(offset_escalado, padreRot);
    Vec3 pos_global = sumarVec3(padrePos, offsetRotado);

    // Calculo de rotaci+on global
    Vec3 rot_global = sumarVec3(padreRot, rot_local_total);

    int tipoFigura = 0; 
    if (nodo->drawFunc == dibujarExtremidad)
        tipoFigura = 1;

    // Calculo de escala global (Escala del padre * Escala local)
    Vec3 esc_global;
    esc_global.x = padreEsc.x * nodo->esc_local.x;
    esc_global.y = padreEsc.y * nodo->esc_local.y;
    esc_global.z = padreEsc.z * nodo->esc_local.z;

    if (nodo->drawFunc != NULL) {
        pushComando(tipoFigura, pos_global, rot_global, esc_global, nodo->material);
    }

    // 5. Procesar Hijos (Pasando mis datos actualizados, incluida MI escala)
    for (int i = 0; i < nodo->num_hijos; i++) {

        addComandos(nodo->hijo[i], pos_global, rot_global, esc_global);
    }
}


// Saca los comandos de la pila y dibuja
void popPila_Y_Dibujado() {
    // Se dibujan primero los últimos hijos agregados.

    while (pila_global.tope != NULL) {
        NodoPila* nodo = popComando();
        RenderCommand cmd = nodo->cmd;

        glPushMatrix();
            glTranslatef(cmd.pos_global.x, cmd.pos_global.y, cmd.pos_global.z);
            
            glRotatef(cmd.rot_global.x, 1, 0, 0);
            glRotatef(cmd.rot_global.y, 0, 1, 0);
            glRotatef(cmd.rot_global.z, 0, 0, 1);
            
            glScalef(cmd.esc_global.x, cmd.esc_global.y, cmd.esc_global.z);

            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
            glColor3fv(cmd.material.diffuse);

            // Dibujar según el tipo
            if (cmd.tipo == 0)
                dibujarCubo();
            else if (cmd.tipo == 1)
                dibujarExtremidad();

            glDisable(GL_COLOR_MATERIAL);
        glPopMatrix();

        free(nodo);
    }
}

void renderizarArbol(Joint *nodo) {
    if (nodo == NULL) return;
    
    limpiarPila();
    
    addComandos(nodo, (Vec3){0,0,0}, (Vec3){0,0,0}, (Vec3){1.0f, 1.0f, 1.0f});
    
    popPila_Y_Dibujado();
}


Joint* crearJoint(char* nombre) {
    Joint* j = (Joint*)malloc(sizeof(Joint));
    strcpy(j->nombre, nombre);
    j->num_hijos = 0;
    j->id = -1;
    j->textura_id = 0;
    j->pos_local = (Vec3){0,0,0};
    j->rot_local = (Vec3){0,0,0};
    j->esc_local = (Vec3){1,1,1};

    j->pos_actual = (Vec3){0,0,0};
    j->rot_actual = (Vec3){0,0,0};



    for(int i=0; i<10; i++)
        j->hijo[i] = NULL;

    j->drawFunc = dibujarCubo;

    j->material.diffuse[0] = 0.5f;
    j->material.diffuse[1] = 0.5f;
    j->material.diffuse[2] = 0.5f;
    j->material.diffuse[3] = 1.0f;

    return j;
}

Joint* armarRobot() {
    
    Joint* torso = crearJoint("Torso");
    torso->pos_local.y = 0.5; 
    torso->esc_local = (Vec3){0.85, 0.75, 0.5}; 
    torso->material.diffuse[0] = 0.0f;
    torso->material.diffuse[1] = 0.0f;
    torso->material.diffuse[2] = 0.0f;

    Joint* cuello = crearJoint("Cuello");
    cuello->pos_local.y = 0.55;
    cuello->esc_local = (Vec3){0.2, 0.2, 0.2};
    cuello->material.diffuse[0] = 1.0f;
    cuello->material.diffuse[1] = 1.0f;
    cuello->material.diffuse[2] = 1.0f;

    Joint* cabeza = crearJoint("Cabeza");
    cabeza->pos_local.y = 1.0; 
    cabeza->esc_local = (Vec3){0.7, 0.7, 0.7}; 
    cabeza->material.diffuse[0] = 1.0;
    cabeza->material.diffuse[1] = 1.0;
    cabeza->material.diffuse[2] = 1.0;

    Joint* ojoIzq = crearJoint("OjoI");
    ojoIzq->pos_local.z = 0.55; 
    ojoIzq->pos_local.x = -0.2; 
    ojoIzq->pos_local.y = 0.05; 
    ojoIzq->esc_local = (Vec3){0.2, 0.35, 0.1}; 
    ojoIzq->material.diffuse[0] = 0.0f;
    ojoIzq->material.diffuse[1] = 0.0f;
    ojoIzq->material.diffuse[2] = 0.0f; 

    Joint* ojoDer = crearJoint("OjoD");
    ojoDer->pos_local.z = 0.55; 
    ojoDer->pos_local.x = 0.2;  
    ojoDer->pos_local.y = 0.05; 
    ojoDer->esc_local = (Vec3){0.2, 0.35, 0.1}; 
    ojoDer->material.diffuse[0] = 0.0f;
    ojoDer->material.diffuse[1] = 0.0f;
    ojoDer->material.diffuse[2] = 0.0f; 

    Joint* brazoI = crearJoint("BrazoI");
    brazoI->pos_local.x = -0.6;
    brazoI->pos_local.y = 0.4;
    brazoI->esc_local = (Vec3){0.25, 0.7, 0.3};
    brazoI->material.diffuse[0] = 1.0f;
    brazoI->material.diffuse[1] = 1.0f;
    brazoI->material.diffuse[2] = 1.0f;

    Joint* brazoD = crearJoint("BrazoD");
    brazoD->pos_local.x = 0.6;
    brazoD->pos_local.y = 0.4;
    brazoD->esc_local = (Vec3){0.25, 0.7, 0.3}; 
    brazoD->material.diffuse[0] = 1.0f;
    brazoD->material.diffuse[1] = 1.0f;
    brazoD->material.diffuse[2] = 1.0f;

    Joint* piernaI = crearJoint("PiernaI");
    piernaI->pos_local.x = -0.25;
    piernaI->pos_local.y = -0.4; 
    piernaI->esc_local = (Vec3){0.25, 0.8, 0.3}; 
    piernaI->material.diffuse[0] = 1.0f;
    piernaI->material.diffuse[1] = 1.0f;
    piernaI->material.diffuse[2] = 1.0f; 

    Joint* piernaD = crearJoint("PiernaD");
    piernaD->pos_local.x = 0.25;
    piernaD->pos_local.y = -0.4; 
    piernaD->esc_local = (Vec3){0.25, 0.8, 0.3}; 
    piernaD->material.diffuse[0] = 1.0f;
    piernaD->material.diffuse[1] = 1.0f;
    piernaD->material.diffuse[2] = 1.0f;

    // Los ojos son hijos de la cabeza
    cabeza->hijo[0] = ojoIzq;
    cabeza->hijo[1] = ojoDer;
    cabeza->num_hijos = 2;

    // El torso es padre de todo lo demás
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

    brazoI->drawFunc = dibujarExtremidad;
    brazoD->drawFunc = dibujarExtremidad;
    piernaI->drawFunc = dibujarExtremidad;
    piernaD->drawFunc = dibujarExtremidad;

    return torso; 
}

void dibujarPiso() {
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glColor3f(0.28f, 0.55f, 0.25f);
    glBegin(GL_QUADS);
        glVertex3f(-200.0, -1.18, -200.0);
        glVertex3f(-200.0, -1.18,  200.0);
        glVertex3f( 200.0, -1.18,  200.0);
        glVertex3f( 200.0, -1.18, -200.0);
    glEnd();
    glEnable(GL_LIGHTING); 
}