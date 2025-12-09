#include "estructuras.h"
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <math.h>

#define PI 3.14159265

GLuint texturaPisoID = 0;
GLuint texturaPechoID = 0;
GLuint texturaParedID = 0;
GLuint texturaCuerpoID = 0;
GLuint texturaCajaID = 0;

GLuint cargarBMP(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("ERROR: No se pudo abrir '%s'\n", filename);
        return 0;
    }
    
    unsigned char header[54];
    fread(header, 1, 54, file);
    
    if (header[0] != 'B' || header[1] != 'M') {
        printf("ERROR: '%s' no es un BMP vÃ¡lido\n", filename);
        fclose(file);
        return 0;
    }
    
    int ancho = *(int*)&(header[18]);
    int largo = *(int*)&(header[22]);
    int data_pos = *(int*)&(header[10]);
    int imagen_tam = ancho * largo * 3;
    
    if (data_pos == 0) data_pos = 54;
    
    // Cargar datos de la imagen
    unsigned char* data = (unsigned char*)malloc(imagen_tam);
    if (!data) {
        printf("ERROR: Memoria insuficiente\n");
        fclose(file);
        return 0;
    }

    fseek(file, data_pos, SEEK_SET);
    fread(data, 1, imagen_tam, file);
    fclose(file);

    // Convertir BGR a RGB
    for (int i = 0; i < imagen_tam; i += 3) {
        unsigned char temp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = temp;
    }

    // Creación de textura
    GLuint textura_id;
    glGenTextures(1, &textura_id);
    glBindTexture(GL_TEXTURE_2D, textura_id);

    // Parámetros de la textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Carga de la textura y generación de mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ancho, largo, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, ancho, largo, GL_RGB, GL_UNSIGNED_BYTE, data);

    free(data);
    printf("Textura '%s' cargada correctamente: %dx%d, ID=%d\n", filename, ancho, largo, textura_id);
    
    return textura_id;
}

void dibujarCubo() {
    float s = 0.5f;
    
    glBegin(GL_QUADS);
        // Frente
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s, -s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s,  s);
        // AtrÃ¡s
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s, -s);
        // Arriba
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s, -s);
        // Abajo
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s,  s);
        // Derecha
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s,  s);
        // Izquierda
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s, -s);
    glEnd();
}

// Hace que el (0,0,0) sea la parte superior del objeto
void dibujarExtremidad() {
    glPushMatrix();
        glTranslatef(0.0, -0.5, 0.0);
        dibujarCubo();
    glPopMatrix();
}

void renderizarJointRecursivo(Joint *nodo) {
    if (nodo == NULL) return;

    glPushMatrix(); // <--- LA CLAVE: Guarda el estado del padre

        // 1. Transformaciones Locales (Se suman a la matriz del padre automáticamente)
        glTranslatef(nodo->pos_local.x + nodo->pos_actual.x,
                     nodo->pos_local.y + nodo->pos_actual.y,
                     nodo->pos_local.z + nodo->pos_actual.z);

        // Rotamos los ejes locales. 
        // El orden importa: Y (Giro) -> X (Caminar) -> Z (Ladear) suele funcionar bien.
        glRotatef(nodo->rot_local.y + nodo->rot_actual.y, 0, 1, 0); 
        glRotatef(nodo->rot_local.x + nodo->rot_actual.x, 1, 0, 0); 
        glRotatef(nodo->rot_local.z + nodo->rot_actual.z, 0, 0, 1); 

        glScalef(nodo->esc_local.x * nodo->esc_actual.x,
                 nodo->esc_local.y * nodo->esc_actual.y,
                 nodo->esc_local.z * nodo->esc_actual.z);

        // 2. Dibujado
        if (nodo->drawFunc != NULL) {
            if (nodo->textura_id > 0) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, nodo->textura_id);
                float blanco[] = {1.0f, 1.0f, 1.0f, 1.0f};
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blanco);
            } else {
                glDisable(GL_TEXTURE_2D);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, nodo->material.diffuse);
            }
            
            nodo->drawFunc();
            glDisable(GL_TEXTURE_2D);
        }

        // 3. Recursividad: Los hijos heredarán esta posición y rotación
        for (int i = 0; i < nodo->num_hijos; i++) {
            renderizarJointRecursivo(nodo->hijo[i]);
        }

    glPopMatrix(); // <--- Regresamos al padre para seguir con el siguiente hermano
}

void renderizarArbol(Joint *nodo) {
    renderizarJointRecursivo(nodo);
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
    j->esc_actual = (Vec3){1.0f, 1.0f, 1.0f};

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
    torso->pos_local.y = -0.3f; 
    torso->esc_local = (Vec3){0.85, 0.75, 0.5}; 
    torso->textura_id = cargarBMP("robot1_pecho.bmp");
    torso->material.diffuse[0] = 0.0f;
    torso->material.diffuse[1] = 0.0f;
    torso->material.diffuse[2] = 0.0f;

    Joint* cuello = crearJoint("Cuello");
    cuello->pos_local.y = 0.55;
    cuello->esc_local = (Vec3){0.2, 0.2, 0.2};
    cuello->textura_id = cargarBMP("robot1_cuerpo.bmp");
    cuello->material.diffuse[0] = 1.0f;
    cuello->material.diffuse[1] = 1.0f;
    cuello->material.diffuse[2] = 1.0f;

    Joint* cabeza = crearJoint("Cabeza");
    cabeza->pos_local.y = 1.0; 
    cabeza->esc_local = (Vec3){0.7, 0.7, 0.7};
    cabeza->textura_id = cargarBMP("robot1_cuerpo.bmp");
    cabeza->material.diffuse[0] = 1.0;
    cabeza->material.diffuse[1] = 1.0;
    cabeza->material.diffuse[2] = 1.0;

    Joint* ojoIzq = crearJoint("OjoI");
    ojoIzq->pos_local.z = 0.55; 
    ojoIzq->pos_local.x = -0.2; 
    ojoIzq->pos_local.y = 0.05; 
    ojoIzq->esc_local = (Vec3){0.2, 0.35, 0.1};
    ojoIzq->textura_id = 0;
    ojoIzq->material.diffuse[0] = 0.0f;
    ojoIzq->material.diffuse[1] = 0.0f;
    ojoIzq->material.diffuse[2] = 0.0f; 

    Joint* ojoDer = crearJoint("OjoD");
    ojoDer->pos_local.z = 0.55; 
    ojoDer->pos_local.x = 0.2;  
    ojoDer->pos_local.y = 0.05; 
    ojoDer->esc_local = (Vec3){0.2, 0.35, 0.1};
    ojoDer->textura_id = 0; 
    ojoDer->material.diffuse[0] = 0.0f;
    ojoDer->material.diffuse[1] = 0.0f;
    ojoDer->material.diffuse[2] = 0.0f; 

    Joint* brazoI = crearJoint("BrazoI");
    brazoI->pos_local.x = -0.6;
    brazoI->pos_local.y = 0.4;
    brazoI->esc_local = (Vec3){0.25, 0.7, 0.3};
    brazoI->textura_id = cargarBMP("robot1_cuerpo.bmp");
    brazoI->material.diffuse[0] = 1.0f;
    brazoI->material.diffuse[1] = 1.0f;
    brazoI->material.diffuse[2] = 1.0f;

    Joint* brazoD = crearJoint("BrazoD");
    brazoD->pos_local.x = 0.6;
    brazoD->pos_local.y = 0.4;
    brazoD->esc_local = (Vec3){0.25, 0.7, 0.3};
    brazoD->textura_id = cargarBMP("robot1_cuerpo.bmp");
    brazoD->material.diffuse[0] = 1.0f;
    brazoD->material.diffuse[1] = 1.0f;
    brazoD->material.diffuse[2] = 1.0f;

    Joint* piernaI = crearJoint("PiernaI");
    piernaI->pos_local.x = -0.25;
    piernaI->pos_local.y = -0.4; 
    piernaI->esc_local = (Vec3){0.25, 0.8, 0.3};
    piernaI->textura_id = cargarBMP("robot1_cuerpo.bmp");
    piernaI->material.diffuse[0] = 1.0f;
    piernaI->material.diffuse[1] = 1.0f;
    piernaI->material.diffuse[2] = 1.0f; 

    Joint* piernaD = crearJoint("PiernaD");
    piernaD->pos_local.x = 0.25;
    piernaD->pos_local.y = -0.4; 
    piernaD->esc_local = (Vec3){0.25, 0.8, 0.3};
    piernaD->textura_id = cargarBMP("robot1_cuerpo.bmp");
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
    
    // IDs para la animación
    torso->id = 0;
    cabeza->id = 1;
    piernaI->id = 2;
    piernaD->id = 3;
    brazoI->id = 4;
    brazoD->id = 5;
    ojoIzq->id = 6;
    ojoDer->id = 7;

    brazoI->drawFunc = dibujarExtremidad;
    brazoD->drawFunc = dibujarExtremidad;
    piernaI->drawFunc = dibujarExtremidad;
    piernaD->drawFunc = dibujarExtremidad;

    return torso; 
}

void dibujarPiso() {
    // Si la textura no se ha cargado, intentar cargarla
    if (texturaPisoID == 0) {
        texturaPisoID = cargarBMP("piso.bmp");
        
        if (texturaPisoID == 0) {
            printf("ERROR: No se pudo cargar la textura 'piso.bmp'\n");
        }
    }

    // Activar texturizado
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaPisoID);
    
    // Propiedades del material
    GLfloat materialColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialColor);
    
    // Dibujar el piso con textura
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        
        glTexCoord2f(0.0f, 0.0f);   glVertex3f(-200.0, -1.18, -200.0);
        glTexCoord2f(0.0f, 10.0f);  glVertex3f(-200.0, -1.18,  200.0);
        glTexCoord2f(10.0f, 10.0f); glVertex3f( 200.0, -1.18,  200.0);
        glTexCoord2f(10.0f, 0.0f);  glVertex3f( 200.0, -1.18, -200.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void dibujarParedes() {
    if (texturaParedID == 0) {
        texturaParedID = cargarBMP("pared.bmp");
        if (texturaParedID == 0 && texturaPisoID != 0) {
            texturaParedID = texturaPisoID;
        }
    }
    
    if (texturaParedID == 0)
        return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaParedID);
    glColor3f(1.0f, 1.0f, 1.0f);

    float ancho = 200.0f;
    float largo = 200.0f;
    float altura = 200.0f;
    
    float y_base = -1.18f;
    float y_top = y_base + altura;

    // Puerta centrada en X = 0
    float puerta_ancho = 25.0f;
    float puerta_alto = 15.0f;
    
    // Factores de textura
    float repH = 20.0f;
    float repV = 5.0f;
    float repPuertaH = (puerta_ancho / ancho) * repH;
    float repDintelV = ((altura - puerta_alto) / altura) * repV;

    glBegin(GL_QUADS);
    
    // Pared trasera (Z negativa)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(-ancho, y_base, -largo);
    glTexCoord2f(repH, 0.0f);   glVertex3f(ancho, y_base, -largo);
    glTexCoord2f(repH, repV);   glVertex3f(ancho, y_top, -largo);
    glTexCoord2f(0.0f, repV);   glVertex3f(-ancho, y_top, -largo);

    // Pared izquierda (X negativa)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(-ancho, y_base, largo);
    glTexCoord2f(repH, 0.0f);   glVertex3f(-ancho, y_base, -largo);
    glTexCoord2f(repH, repV);   glVertex3f(-ancho, y_top, -largo);
    glTexCoord2f(0.0f, repV);   glVertex3f(-ancho, y_top, largo);

    // Pared derecha (X positiva)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(ancho, y_base, -largo);
    glTexCoord2f(repH, 0.0f);   glVertex3f(ancho, y_base, largo);
    glTexCoord2f(repH, repV);   glVertex3f(ancho, y_top, largo);
    glTexCoord2f(0.0f, repV);   glVertex3f(ancho, y_top, -largo);

    // Pared frontal con puerta (Z positiva)
    glNormal3f(0.0f, 0.0f, -1.0f);
    
    // Izquierda de la puerta
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(ancho, y_base, largo);
    glTexCoord2f(repH/2 - repPuertaH, 0.0f);    glVertex3f(puerta_ancho, y_base, largo);
    glTexCoord2f(repH/2 - repPuertaH, repV);    glVertex3f(puerta_ancho, y_top, largo);
    glTexCoord2f(0.0f, repV);   glVertex3f(ancho, y_top, largo);

    // Derecha de la puerta
    glTexCoord2f(repH/2 + repPuertaH, 0.0f);    glVertex3f(-puerta_ancho, y_base, largo);
    glTexCoord2f(repH, 0.0f);   glVertex3f(-ancho, y_base, largo);
    glTexCoord2f(repH, repV);   glVertex3f(-ancho, y_top, largo);
    glTexCoord2f(repH/2 + repPuertaH, repV);    glVertex3f(-puerta_ancho, y_top, largo);

    // Dintel (parte superior de la puerta)
    float y_puerta_top = y_base + puerta_alto;
    glTexCoord2f(repH/2 - repPuertaH, repV - repDintelV);   glVertex3f(puerta_ancho, y_puerta_top, largo);
    glTexCoord2f(repH/2 + repPuertaH, repV - repDintelV);   glVertex3f(-puerta_ancho, y_puerta_top, largo);
    glTexCoord2f(repH/2 + repPuertaH, repV);    glVertex3f(-puerta_ancho, y_top, largo);
    glTexCoord2f(repH/2 - repPuertaH, repV);    glVertex3f(puerta_ancho, y_top, largo);

    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void dibujarCajas(int numFilas, int numCols, int faltaFila, int faltaCol) {
    if (texturaCajaID == 0) {
        texturaCajaID = cargarBMP("caja.bmp");
    }

    if (texturaCajaID > 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturaCajaID);
        float blanco[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blanco);
    } else {
        glDisable(GL_TEXTURE_2D);
        float colorCarton[] = {0.76f, 0.60f, 0.42f, 1.0f};
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, colorCarton);
    }

    float tamano = 1.0f;
    float separacion = 1.5f; 
    
    float anchoTotal = (numCols * separacion);
    float inicioX = -(anchoTotal / 2.0f) + (separacion / 2.0f);

    float inicioZ = 2.5f; 

    for (int fila = 0; fila < numFilas; fila++) {       
        for (int col = 0; col < numCols; col++) {     
            
            if (fila == faltaFila && col == faltaCol) continue;

            glPushMatrix();
                float x = inicioX + (col * separacion);
                float y = -0.68f; 
                float z = inicioZ + (fila * separacion); 

                glTranslatef(x, y, z);
                
                glBegin(GL_QUADS);
                    glNormal3f(0.0f, 0.0f, 1.0f);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f,  0.5f);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f,  0.5f,  0.5f);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f,  0.5f);
                    
                    glNormal3f(0.0f, 0.0f, -1.0f);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f,  0.5f, -0.5f);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
                    
                    glNormal3f(0.0f, 1.0f, 0.0f);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f,  0.5f,  0.5f);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f,  0.5f,  0.5f);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f,  0.5f, -0.5f);
                    
                    glNormal3f(0.0f, -1.0f, 0.0f);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f, -0.5f, -0.5f);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f,  0.5f);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);
                    
                    glNormal3f(1.0f, 0.0f, 0.0f);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f,  0.5f, -0.5f);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f,  0.5f,  0.5f);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f,  0.5f);
                    
                    glNormal3f(-1.0f, 0.0f, 0.0f);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f,  0.5f);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);
                glEnd();
                
            glPopMatrix();
        }
    }

    glDisable(GL_TEXTURE_2D);
}

void renderizarEscena(NodoEscena *nodo) {
    if (nodo == NULL || !nodo->visible) return;

    glPushMatrix();
        glTranslatef(nodo->pos.x, nodo->pos.y, nodo->pos.z);
        glRotatef(nodo->rot.x, 1, 0, 0);
        glRotatef(nodo->rot.y, 0, 1, 0);
        glRotatef(nodo->rot.z, 0, 0, 1);
        glScalef(nodo->esc.x, nodo->esc.y, nodo->esc.z);

        if (nodo->tipo == 1) { 
            if (nodo->pos.y < -1.0)
                dibujarPiso();
            else if (nodo->pos.z > 1.0) {
                dibujarCajas(5, 12, 2, 4); 
            }
            else 
                dibujarParedes(); 
        } else if (nodo->tipo == 2) {
            if (nodo->dato != NULL) {
                renderizarArbol((Joint*)nodo->dato);
            }
        }

        for (int i = 0; i < nodo->num_hijos; i++) {
            renderizarEscena(nodo->hijos[i]);
        }

    glPopMatrix();
}