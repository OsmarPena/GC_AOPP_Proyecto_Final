#include "estructuras.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Importante: Traemos la variable global de main.c para poder detener la app
extern int animando; 
extern Joint* personaje;

ColaAcciones cola_global;
AccionEscena accion_act; 
int hay_accion = 0;   
Camara camara_escena;

NodoEscena *escena_raiz = NULL;         
ListaObjetos lista_objetos_activos;     
ListaObjetos lista_objetos_estaticos;   
PilaHistorial pila_historial;           

static float tiempoActual = 0.0f;
static float tiempoGlobal = 0.0f;
static float velocidadAnimacion = 1.0f;
static Animacion* animacion_principal = NULL;
static Camara cam_inic;
static Camara camEnd;
static int id_contador = 0;

void iniciarLista(ListaObjetos *lista) {
    lista->cabeza = NULL; lista->count = 0;
}

void agregarALista(ListaObjetos *lista, NodoEscena *obj) {
    NodoListaObjeto *nuevo = (NodoListaObjeto*)malloc(sizeof(NodoListaObjeto));
    nuevo->objeto = obj; nuevo->sgt = lista->cabeza;
    lista->cabeza = nuevo; lista->count++;
}

void iniciarPilaHistorial() {
    pila_historial.tope = NULL;
    pila_historial.cont = 0;
}

void pushHistorial(int obj_id, Vec3 pos, Vec3 rot) {
    NodoPilaHistorial *nuevo = (NodoPilaHistorial*)malloc(sizeof(NodoPilaHistorial));
    nuevo->objeto_id = obj_id;
    nuevo->pos_anterior = pos;
    nuevo->rot_anterior = rot;
    nuevo->sgt = pila_historial.tope;
    pila_historial.tope = nuevo;
    pila_historial.cont++;
}

NodoPilaHistorial* popHistorial() {
    if (pila_historial.tope == NULL)
        return NULL;
    NodoPilaHistorial *aux = pila_historial.tope;
    pila_historial.tope = aux->sgt;
    pila_historial.cont--;
    return aux;
}

NodoEscena* crearNodoEscena(int tipo, Vec3 pos, Vec3 rot, Vec3 esc) {
    NodoEscena *nodo = (NodoEscena*)malloc(sizeof(NodoEscena));
    nodo->id = id_contador++;
    nodo->tipo = tipo;
    nodo->pos = pos;
    nodo->rot = rot;
    nodo->esc = esc;
    nodo->dato = NULL;
    nodo->visible = true;
    nodo->padre = NULL;
    nodo->hijos = NULL;
    nodo->num_hijos = 0;
    return nodo;
}

void agregarHijoEscena(NodoEscena *padre, NodoEscena *hijo) {
    padre->num_hijos++;
    padre->hijos = (NodoEscena**)realloc(padre->hijos, sizeof(NodoEscena*) * padre->num_hijos);
    padre->hijos[padre->num_hijos - 1] = hijo; hijo->padre = padre;
}

void resetearArbol(Joint* nodo) {
    if (nodo == NULL) return;
    nodo->pos_actual = (Vec3){0,0,0};
    nodo->rot_actual = (Vec3){0,0,0};
    nodo->esc_actual = (Vec3){1.0f, 1.0f, 1.0f};
    for (int i = 0; i < nodo->num_hijos; i++) resetearArbol(nodo->hijo[i]);
}

KeyFrame* crearKeyFrame(float tiempo) {
    KeyFrame* kf = (KeyFrame*)malloc(sizeof(KeyFrame));
    kf->timeStamp = tiempo;
    kf->poses = NULL;
    kf->num_poses = 0;
    kf->sgt = NULL;
    return kf;
}

void putPoseToKeyFrame(KeyFrame* kf, int joint_id, Vec3 pos, Vec3 rot, Vec3 esc) {
    kf->num_poses++;
    kf->poses = (JointTransform*)realloc(kf->poses, sizeof(JointTransform) * kf->num_poses);
    int id = kf->num_poses - 1;
    kf->poses[id].joint_id = joint_id;
    kf->poses[id].pos = pos;
    kf->poses[id].rot = rot;
    kf->poses[id].esc = esc;
}

void agregarKeyFrame(Animacion* anim, KeyFrame* kf) {
    if (anim->frames == NULL) anim->frames = kf;
    else {
        KeyFrame* temp = anim->frames;
        while (temp->sgt != NULL) temp = temp->sgt;
        temp->sgt = kf;
    }
}

Vec3 interpolarVec3(Vec3 a, Vec3 b, float t) {
    return (Vec3){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

void aplicarInterpolacion(Joint* robot, int joint_id, Vec3 pos, Vec3 rot, Vec3 esc) {
    if (robot == NULL) return;
    if (robot->id == joint_id) {
        robot->pos_actual = pos;
        robot->rot_actual = rot;
        robot->esc_actual = esc;
        return;
    }
    for (int i = 0; i < robot->num_hijos; i++) 
        aplicarInterpolacion(robot->hijo[i], joint_id, pos, rot, esc);
}

void getKeyFramesAdyacentes(Animacion* anim, float tiempo, KeyFrame** ant, KeyFrame** sgt) {
    *ant = NULL; *sgt = NULL;
    if (anim == NULL || anim->frames == NULL) return;

    KeyFrame* kf = anim->frames;
    while (kf != NULL) {
        if (kf->timeStamp <= tiempo) *ant = kf;
        if (kf->timeStamp > tiempo && *sgt == NULL) *sgt = kf;
        kf = kf->sgt;
    }
    if (*sgt == NULL && *ant != NULL) *sgt = *ant;
    if (*ant == NULL) *ant = anim->frames;
}

void iniciarCola() { 
    cola_global.frente = NULL; cola_global.fondo = NULL; hay_accion = 0;
}

void limpiarCola() {
    while(cola_global.frente != NULL) {
        NodoCola* temp = cola_global.frente;
        cola_global.frente = temp->sgt;
        free(temp);
    }
    cola_global.fondo = NULL; hay_accion = 0;
}

void encolarEscena(Animacion* anim, Camara cam_inicial, Camara cam_final, int corte) {
    NodoCola* nuevo = (NodoCola*)malloc(sizeof(NodoCola));
    nuevo->accion.anim_actual = anim;
    nuevo->accion.mov_camara_inicio = cam_inicial; 
    nuevo->accion.mov_camara_fin = cam_final;
    nuevo->accion.duracion = anim->duracion;
    nuevo->accion.es_corte = corte;
    nuevo->sgt = NULL;

    if (cola_global.fondo == NULL) {
        cola_global.frente = nuevo; cola_global.fondo = nuevo;
    } else { 
        cola_global.fondo->sgt = nuevo; cola_global.fondo = nuevo;
    }
}

void transicionesDeEscenas() {
    if (!hay_accion || tiempoActual >= accion_act.duracion) {
        if (cola_global.frente != NULL) {
            NodoCola* temp = cola_global.frente;
            accion_act = temp->accion;
            animacion_principal = accion_act.anim_actual;
            
            if (!hay_accion) cam_inic = accion_act.mov_camara_inicio;
            else if (accion_act.es_corte == 1) cam_inic = accion_act.mov_camara_inicio;
            else cam_inic = camara_escena; 
            
            camEnd = accion_act.mov_camara_fin;
            tiempoActual = 0.0f;
            hay_accion = 1;
            cola_global.frente = temp->sgt;
            if (cola_global.frente == NULL) cola_global.fondo = NULL;
            free(temp);
        } else {
            animando = 0; 
            printf("FIN DE LA ANIMACION\n");
        }
    }
}

void actualizarCamara() {
    if (!hay_accion) return;
    float t = tiempoActual / accion_act.duracion;
    if (t > 1.0f) t = 1.0f;
    camara_escena.eye = interpolarVec3(cam_inic.eye, camEnd.eye, t);
    camara_escena.center = interpolarVec3(cam_inic.center, camEnd.center, t);
    camara_escena.up = (Vec3){0,1,0};
}

void actualizarAnimacion(Joint* robot) {
    transicionesDeEscenas();
    if (animando == 0) return;

    actualizarCamara();
    if (animacion_principal == NULL || robot == NULL) return;
    
    tiempoActual += (1.0f / 24.0f) * velocidadAnimacion;
    tiempoGlobal += (1.0f / 24.0f) * velocidadAnimacion;

    if (tiempoActual > animacion_principal->duracion) tiempoActual = animacion_principal->duracion;
    
    KeyFrame *ant, *sgt;
    getKeyFramesAdyacentes(animacion_principal, tiempoActual, &ant, &sgt);
    if (ant == NULL || sgt == NULL) return;
    
    float t = 0.0f;
    if (sgt->timeStamp != ant->timeStamp) 
        t = (tiempoActual - ant->timeStamp) / (sgt->timeStamp - ant->timeStamp);
    
    for (int i = 0; i < ant->num_poses; i++) {
        JointTransform poseAnt = ant->poses[i]; 
        JointTransform poseSig = poseAnt; 
        for (int j = 0; j < sgt->num_poses; j++) {
            if (sgt->poses[j].joint_id == poseAnt.joint_id) { 
                poseSig = sgt->poses[j]; break; 
            }
        }
        Vec3 posInt = interpolarVec3(poseAnt.pos, poseSig.pos, t);
        Vec3 rotInt = interpolarVec3(poseAnt.rot, poseSig.rot, t);
        Vec3 escInt = interpolarVec3(poseAnt.esc, poseSig.esc, t); 
        aplicarInterpolacion(robot, poseAnt.joint_id, posInt, rotInt, escInt);
    }
}

void caminar(Animacion* anim, float* t, Vec3* pos, float ang, float dist, float vel, float paso_largo) {
    float rad = ang * (3.14159265f / 180.0f);
    Vec3 delta = { sin(rad) * dist, 0, cos(rad) * dist };
    Vec3 fin = { pos->x + delta.x, pos->y, pos->z + delta.z };
    
    int pasos = (int)(dist / paso_largo);
    if(pasos < 1) pasos = 1;
    
    float dt = vel; 
    Vec3 norm = {1,1,1};
    
    for(int i=0; i<pasos; i++) {
        *t += dt;
        float prog = (float)(i+1)/(float)pasos;
        Vec3 curPos = interpolarVec3(*pos, fin, prog);
        
        KeyFrame* kf = crearKeyFrame(*t);
        putPoseToKeyFrame(kf, 0, curPos, (Vec3){0, ang, 0}, norm);
        
        Vec3 pA = {45,0,0}, pAt = {-45,0,0};
        if (i%2==0) {
            putPoseToKeyFrame(kf, 2, (Vec3){0,0,0}, pA, norm);
            putPoseToKeyFrame(kf, 3, (Vec3){0,0,0}, pAt, norm);
            putPoseToKeyFrame(kf, 4, (Vec3){0,0,0}, pAt, norm);
            putPoseToKeyFrame(kf, 5, (Vec3){0,0,0}, pA, norm);
        } else {
            putPoseToKeyFrame(kf, 2, (Vec3){0,0,0}, pAt, norm);
            putPoseToKeyFrame(kf, 3, (Vec3){0,0,0}, pA, norm);
            putPoseToKeyFrame(kf, 4, (Vec3){0,0,0}, pA, norm);
            putPoseToKeyFrame(kf, 5, (Vec3){0,0,0}, pAt, norm);
        }
        agregarKeyFrame(anim, kf);
    }
    *pos = fin;
    
    *t += 0.1f;
    KeyFrame* kfQ = crearKeyFrame(*t);
    putPoseToKeyFrame(kfQ, 0, *pos, (Vec3){0, ang, 0}, norm);
    putPoseToKeyFrame(kfQ, 2, (Vec3){0,0,0}, (Vec3){0,0,0}, norm);
    putPoseToKeyFrame(kfQ, 3, (Vec3){0,0,0}, (Vec3){0,0,0}, norm);
    putPoseToKeyFrame(kfQ, 4, (Vec3){0,0,0}, (Vec3){0,0,0}, norm);
    putPoseToKeyFrame(kfQ, 5, (Vec3){0,0,0}, (Vec3){0,0,0}, norm);
    agregarKeyFrame(anim, kfQ);
}

void rotar(Animacion* anim, float* t, Vec3 pos, float* angActual, float angDestino, float duracion) {
    *t += duracion;
    *angActual = angDestino;
    KeyFrame* kf = crearKeyFrame(*t);
    putPoseToKeyFrame(kf, 0, pos, (Vec3){0, *angActual, 0}, (Vec3){1,1,1});
    agregarKeyFrame(anim, kf);
}

Animacion* escena1() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Acercamiento");
    anim->duracion = 8.5f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);
    
    KeyFrame* kf2 = crearKeyFrame(2.0f);
    putPoseToKeyFrame(kf2, 1, (Vec3){0,0,0}, (Vec3){0, -50, 0}, normal);
    agregarKeyFrame(anim, kf2);
    
    KeyFrame* kf5 = crearKeyFrame(5.0f);
    putPoseToKeyFrame(kf5, 1, (Vec3){0,0,0}, (Vec3){0, 50, 0}, normal);
    agregarKeyFrame(anim, kf5);
    
    KeyFrame* kf8 = crearKeyFrame(8.0f);
    putPoseToKeyFrame(kf8, 1, (Vec3){0,0,0}, (Vec3){0, 0, 0}, normal);
    putPoseToKeyFrame(kf8, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf8, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf8);
    return anim;
}

Animacion* escena2() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Parpadeo del robot");
    anim->duracion = 4.0f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};
    Vec3 aplastado = {1.0f, 0.1f, 1.0f};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kf1 = crearKeyFrame(1.0f);
    putPoseToKeyFrame(kf1, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf1, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf1);

    KeyFrame* kf2 = crearKeyFrame(1.25f);
    putPoseToKeyFrame(kf2, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf2, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf2);

    KeyFrame* kf3 = crearKeyFrame(1.5f);
    putPoseToKeyFrame(kf3, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf3, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf3);

    KeyFrame* kf4 = crearKeyFrame(2.2f);
    putPoseToKeyFrame(kf4, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf4, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf4);

    KeyFrame* kf5 = crearKeyFrame(2.45f);
    putPoseToKeyFrame(kf5, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf5, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf5);

    KeyFrame* kf6 = crearKeyFrame(2.7f);
    putPoseToKeyFrame(kf6, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf6, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf6);

    KeyFrame* kfEnd = crearKeyFrame(4.5f);
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena3() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "POV a la izquierda");
    anim->duracion = 2.5f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};

    KeyFrame* kf0 = crearKeyFrame(0.0f); 
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kfEnd = crearKeyFrame(2.5f); 
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, (Vec3){0, -30, 0}, normal);
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena4() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Pausa Izquierda");
    anim->duracion = 1.5f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0, -30, 0}, normal);
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kfEnd = crearKeyFrame(1.5f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, (Vec3){0, -30, 0}, normal);
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena5() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "POV a la derecha");
    anim->duracion = 5.0f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};

    KeyFrame* kf0 = crearKeyFrame(0.0f); 
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0, -30, 0}, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kfEnd = crearKeyFrame(5.0f); 
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, (Vec3){0, 30, 0}, normal);
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena6() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Pausa Derecha");
    anim->duracion = 1.5f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0, 30, 0}, normal);
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kfEnd = crearKeyFrame(1.5f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, (Vec3){0, 30, 0}, normal);
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena7() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Robot cierra ojos");
    anim->duracion = 6.5f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};
    Vec3 aplastado = {1.0f, 0.1f, 1.0f};
    Vec3 cabeza_arriba = {-25.0f, 0.0f, 0.0f}; 
    Vec3 cabeza_lado = {0.0f, 30.0f, 0.0f}; 

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, cabeza_lado, normal);
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kf1 = crearKeyFrame(1.5f);
    putPoseToKeyFrame(kf1, 1, (Vec3){0,0,0}, cabeza_lado, normal);
    putPoseToKeyFrame(kf1, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf1, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf1);

    KeyFrame* kf2 = crearKeyFrame(1.7f);
    putPoseToKeyFrame(kf2, 1, (Vec3){0,0,0}, cabeza_lado, normal);
    putPoseToKeyFrame(kf2, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf2, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf2);

    KeyFrame* kf3 = crearKeyFrame(1.9f);
    putPoseToKeyFrame(kf3, 1, (Vec3){0,0,0}, cabeza_lado, normal);
    putPoseToKeyFrame(kf3, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf3, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf3);

    KeyFrame* kf4 = crearKeyFrame(3.5f);
    putPoseToKeyFrame(kf4, 1, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf4);

    KeyFrame* kf5 = crearKeyFrame(4.5f);
    putPoseToKeyFrame(kf5, 1, (Vec3){0,0,0}, cabeza_arriba, normal); 
    putPoseToKeyFrame(kf5, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf5, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf5);

    KeyFrame* kfEnd = crearKeyFrame(6.5f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, cabeza_arriba, normal); 
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena8() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Giro de cuello");
    anim->duracion = 2.0f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};
    Vec3 aplastado = {1.0f, 0.1f, 1.0f};

    Vec3 cabeza_inicio = {-25.0f, 0.0f, 0.0f};   
    Vec3 cabeza_fin    = {-25.0f, -20.0f, 0.0f}; 

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, cabeza_inicio, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kfEnd = crearKeyFrame(2.0f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, cabeza_fin, normal); 
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena9() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Pausa");
    anim->duracion = 1.5f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};
    Vec3 aplastado = {1.0f, 0.1f, 1.0f};
    Vec3 cabeza_pose = {-25.0f, -20.0f, 0.0f}; 

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kfEnd = crearKeyFrame(1.5f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena10() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "POV Tenso");
    anim->duracion = 4.0f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};
    Vec3 aplastado = {1.0f, 0.1f, 1.0f};
    Vec3 cabeza_pose = {25.0f, -20.0f, 0.0f};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado); 
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf0);
    
    KeyFrame* kfEnd = crearKeyFrame(4.0f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena11() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Susto");
    anim->duracion = 2.0f; 
    anim->frames = NULL;
    Vec3 normal = {1,1,1};
    Vec3 aplastado = {1.0f, 0.1f, 1.0f};
    Vec3 cabeza_arriba = {-25.0f, -20.0f, 0.0f};
    Vec3 cabeza_pose = {0.0f, -20.0f, 0.0f};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, cabeza_arriba, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kf1 = crearKeyFrame(0.2f);
    putPoseToKeyFrame(kf1, 1, (Vec3){0,0,0}, cabeza_arriba, normal); 
    putPoseToKeyFrame(kf1, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kf1, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf1);

    KeyFrame* kfEnd = crearKeyFrame(0.3f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena12() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Robot en shock");
    anim->duracion = 4.0f;
    anim->frames = NULL;
    Vec3 normal = {1,1,1};
    Vec3 shock = {0.3f, 0.4f, 1.0f};
    Vec3 cabeza_pose = {0.0f, -20.0f, 0.0f};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    KeyFrame* kf1 = crearKeyFrame(0.2f);
    putPoseToKeyFrame(kf1, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kf1, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, shock); 
    putPoseToKeyFrame(kf1, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, shock);
    agregarKeyFrame(anim, kf1);

    KeyFrame* kfEnd = crearKeyFrame(3.0f);
    putPoseToKeyFrame(kfEnd, 1, (Vec3){0,0,0}, cabeza_pose, normal); 
    putPoseToKeyFrame(kfEnd, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, shock);
    putPoseToKeyFrame(kfEnd, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, shock);
    agregarKeyFrame(anim, kfEnd);
    return anim;
}

Animacion* escena13() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Busqueda extrema");
    anim->duracion = 30.0f; 
    anim->frames = NULL;

    Vec3 normal = {1,1,1};
    Vec3 shock = {0.3f, 0.4f, 1.0f};
    
    Vec3 pos = {0,0,0};
    float ang = 0;
    float t = 0.0f;
    
    float vel_run = 0.15f; 
    float vel_turn = 0.2f; 
    float zancada = 1.2f;  

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 0, pos, (Vec3){0, ang, 0}, normal);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, shock);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, shock);
    agregarKeyFrame(anim, kf0);

    rotar(anim, &t, pos, &ang, 180.0f, vel_turn);
    caminar(anim, &t, &pos, ang, 10.0f, vel_run, zancada);

    rotar(anim, &t, pos, &ang, -90.0f, vel_turn);
    caminar(anim, &t, &pos, ang, 15.0f, vel_run, zancada);

    rotar(anim, &t, pos, &ang, 180.0f, vel_turn);
    caminar(anim, &t, &pos, ang, 30.0f, vel_run, zancada);

    rotar(anim, &t, pos, &ang, 90.0f, vel_turn);
    caminar(anim, &t, &pos, ang, 5.0f, vel_run, zancada);

    rotar(anim, &t, pos, &ang, -90.0f, vel_turn);
    caminar(anim, &t, &pos, ang, 10.0f, vel_run, zancada);

    return anim;
}

void cargarSecuenciaDeComandos() {
    limpiarCola();

    Vec3 target_hueco = {-1.5, 0.5, 5.0};
    Vec3 pov_pos = {0.0f, 1.0f, 0.2f};
    Vec3 pov_zoom_pos = {-0.5f, 0.8f, 1.5f};

    Camara cam_lejos = {{0.0, 6.0, 12.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    Camara cam_cerca = {{0.0, 1.2, 3.0}, {0.0, 0.6, 0.0}, {0.0, 1.0, 0.0}};
    Camara cam_pov_centro = {pov_pos, { 0.0, 0.5, 5.0}, {0.0, 1.0, 0.0}};
    Camara cam_pov_izq    = {pov_pos, {-2.0, 0.5, 5.0}, {0.0, 1.0, 0.0}}; 
    Camara cam_pov_der    = {pov_pos, { 2.0, 0.5, 5.0}, {0.0, 1.0, 0.0}}; 
    Camara cam_pov_hueco      = {pov_pos, target_hueco, {0.0, 1.0, 0.0}};
    Camara cam_pov_hueco_zoom = {pov_zoom_pos, target_hueco, {0.0, 1.0, 0.0}};

    Camara cam_frente_inicio = {{0.0, 4.0, 9.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    Camara cam_aerea_fin     = {{0.0, 25.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};

    encolarEscena(escena1(), cam_lejos, cam_cerca, 0);
    encolarEscena(escena2(), cam_cerca, cam_cerca, 1);
    
    encolarEscena(escena3(), cam_pov_centro, cam_pov_izq, 1); 
    encolarEscena(escena4(), cam_pov_izq, cam_pov_izq, 1);
    encolarEscena(escena5(), cam_pov_izq, cam_pov_der, 0); 
    encolarEscena(escena6(), cam_pov_der, cam_pov_der, 1);

    encolarEscena(escena7(), cam_cerca, cam_cerca, 1); 
    encolarEscena(escena8(), cam_cerca, cam_cerca, 1);
    encolarEscena(escena9(), cam_cerca, cam_cerca, 1);
    
    encolarEscena(escena10(), cam_pov_hueco, cam_pov_hueco, 1); 
    encolarEscena(escena11(), cam_cerca, cam_cerca, 1);

    Animacion* anim_zoom = escena11(); 
    anim_zoom->duracion = 0.8f; 
    strcpy(anim_zoom->nombre, "Parte 8A: Zoom Rapido");
    encolarEscena(anim_zoom, cam_pov_hueco, cam_pov_hueco_zoom, 1); 

    Animacion* anim_hold = escena11();
    anim_hold->duracion = 1.0f;
    strcpy(anim_hold->nombre, "Parte 8B: Hold Dramatico");
    encolarEscena(anim_hold, cam_pov_hueco_zoom, cam_pov_hueco_zoom, 0); 

    encolarEscena(escena12(), cam_cerca, cam_cerca, 1);

    encolarEscena(escena13(), cam_frente_inicio, cam_aerea_fin, 1);
}

void construirEscenario() {
    iniciarPilaHistorial();
    iniciarLista(&lista_objetos_activos);
    iniciarLista(&lista_objetos_estaticos);
    
    escena_raiz = crearNodoEscena(0, (Vec3){0,0,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    NodoEscena *nodo_personaje = crearNodoEscena(2, (Vec3){0,0.5,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    NodoEscena *nodo_piso = crearNodoEscena(1, (Vec3){0,-1.18,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    NodoEscena *nodo_paredes = crearNodoEscena(1, (Vec3){0,0,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    NodoEscena *nodo_cajas = crearNodoEscena(1, (Vec3){0,0,2.5}, (Vec3){0,0,0}, (Vec3){1,1,1});
    
    agregarHijoEscena(escena_raiz, nodo_piso);
    agregarHijoEscena(escena_raiz, nodo_paredes);
    agregarHijoEscena(escena_raiz, nodo_cajas);
    agregarHijoEscena(escena_raiz, nodo_personaje);
    
    agregarALista(&lista_objetos_estaticos, nodo_piso);
    agregarALista(&lista_objetos_estaticos, nodo_paredes);
    agregarALista(&lista_objetos_estaticos, nodo_cajas);
    agregarALista(&lista_objetos_activos, nodo_personaje);
}

void iniciarGuion() {
    iniciarCola();
    construirEscenario();
    cargarSecuenciaDeComandos();
    if (cola_global.frente != NULL) {
        camara_escena = cola_global.frente->accion.mov_camara_inicio;
    }
}

void reiniciarAnimacion() {
    tiempoActual = 0.0f;
    tiempoGlobal = 0.0f;
    hay_accion = 0;

    if (personaje != NULL) {
        resetearArbol(personaje);
    }
    while (pila_historial.tope != NULL) {
        NodoPilaHistorial *n = popHistorial(); free(n);
    }
    cargarSecuenciaDeComandos(); 
    if (cola_global.frente != NULL) {
        camara_escena = cola_global.frente->accion.mov_camara_inicio;
    }
}

float obtenerTiempoActual() {
    return tiempoGlobal;
}