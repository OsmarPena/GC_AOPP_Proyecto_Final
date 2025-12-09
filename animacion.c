#include "estructuras.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

// Ajuste de altura (Torso)
const float Y_BASE_ROBOT = -0.38f; 

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

void recorrerArbolEscena(NodoEscena *nodo, int profundidad) {
    if (nodo == NULL)
        return;
    for (int i = 0; i < nodo->num_hijos; i++)
        recorrerArbolEscena(nodo->hijos[i], profundidad + 1);
}

// Funciones de key frames
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
    if (anim == NULL || anim->frames == NULL)
        return;

    KeyFrame* kf = anim->frames;
    while (kf != NULL) {
        if (kf->timeStamp <= tiempo)
            *ant = kf;
        if (kf->timeStamp > tiempo && *sgt == NULL)
            *sgt = kf;

        kf = kf->sgt;
    }

    if (*sgt == NULL && *ant != NULL)
        *sgt = *ant;

    if (*ant == NULL)
        *ant = anim->frames;
}

void iniciarCola() { 
    cola_global.frente = NULL;
    cola_global.fondo = NULL;
    hay_accion = 0;
}

void limpiarCola() {
    while(cola_global.frente != NULL) {
        NodoCola* temp = cola_global.frente;
        cola_global.frente = temp->sgt;
        free(temp);
    }

    cola_global.fondo = NULL;
    hay_accion = 0;
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
        cola_global.frente = nuevo;
        cola_global.fondo = nuevo;
    } else { 
        cola_global.fondo->sgt = nuevo;
        cola_global.fondo = nuevo;
    }
}

void transicionesDeEscenas() {
    if (!hay_accion || tiempoActual >= accion_act.duracion) {
        if (cola_global.frente != NULL) {
            NodoCola* temp = cola_global.frente;
            accion_act = temp->accion;
            animacion_principal = accion_act.anim_actual;
            
            if (!hay_accion)
                cam_inic = accion_act.mov_camara_inicio;
            else if (accion_act.es_corte == 1)
                cam_inic = accion_act.mov_camara_inicio;
            else
                cam_inic = camara_escena; 
            
            camEnd = accion_act.mov_camara_fin;
            tiempoActual = 0.0f;
            hay_accion = 1;
            cola_global.frente = temp->sgt;

            if (cola_global.frente == NULL)
                cola_global.fondo = NULL;

            free(temp);
        } else {
            tiempoActual = 0.0f; 
        }
    }
}

void actualizarCamara() {
    if (!hay_accion)
        return;

    float t = tiempoActual / accion_act.duracion;

    if (t > 1.0f)
        t = 1.0f;

    camara_escena.eye = interpolarVec3(cam_inic.eye, camEnd.eye, t);
    camara_escena.center = interpolarVec3(cam_inic.center, camEnd.center, t);
    camara_escena.up = (Vec3){0,1,0};
}

void actualizarAnimacion(Joint* robot) {
    transicionesDeEscenas();
    actualizarCamara();
    if (animacion_principal == NULL || robot == NULL)
        return;
    
    tiempoActual += (1.0f / 24.0f) * velocidadAnimacion;
    tiempoGlobal += (1.0f / 24.0f) * velocidadAnimacion;

    if (tiempoActual > animacion_principal->duracion)
        tiempoActual = animacion_principal->duracion;
    
    KeyFrame *ant, *sgt;
    getKeyFramesAdyacentes(animacion_principal, tiempoActual, &ant, &sgt);
    if (ant == NULL || sgt == NULL)
        return;
    
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

// ESCENAS

// 1. El robot mira a los lados mientras la cámara viaja
Animacion* escena_parte1_acercamiento() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Parte 1: Acercamiento");
    anim->duracion = 8.0f; 
    anim->frames = NULL;
    
    // Escala 'normal'
    Vec3 normal = {1,1,1};

    KeyFrame* kf0 = crearKeyFrame(0.0f);
    // Añadimos 'normal' al final de cada llamada
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal); 
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);
    
    // --- 2s: Mira Izquierda ---
    KeyFrame* kf2 = crearKeyFrame(2.0f);
    putPoseToKeyFrame(kf2, 1, (Vec3){0,0,0}, (Vec3){0, -50, 0}, normal);
    agregarKeyFrame(anim, kf2);
    
    // --- 5s: Mira Derecha ---
    KeyFrame* kf5 = crearKeyFrame(5.0f);
    putPoseToKeyFrame(kf5, 1, (Vec3){0,0,0}, (Vec3){0, 50, 0}, normal);
    agregarKeyFrame(anim, kf5);
    
    // --- 8s: Termina mirando al centro ---
    KeyFrame* kf8 = crearKeyFrame(8.0f);
    putPoseToKeyFrame(kf8, 1, (Vec3){0,0,0}, (Vec3){0, 0, 0}, normal);
    putPoseToKeyFrame(kf8, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf8, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf8);
    
    return anim;
}

// 2. Robot quieto y doble parpadeo
Animacion* escena_parte2_parpadeo() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Parte 2: Parpadeo con Escala");
    anim->duracion = 4.5f; 
    anim->frames = NULL;

    Vec3 normal = {1,1,1};
    Vec3 aplastado = {1.0f, 0.1f, 1.0f}; // Aplastado al 10% en Y

    // --- 0.0s: Inicio ---
    KeyFrame* kf0 = crearKeyFrame(0.0f);
    // Se tienen que pasar 3 vectores: Pos, Rot, Esc
    putPoseToKeyFrame(kf0, 1, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf0, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf0);

    // Primer parpadeo
    // 1.0s: Empieza normal
    KeyFrame* kf1 = crearKeyFrame(1.0f);
    putPoseToKeyFrame(kf1, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf1, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf1);

    // 1.25s: Ojos aplastados
    KeyFrame* kf2 = crearKeyFrame(1.25f);
    putPoseToKeyFrame(kf2, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    putPoseToKeyFrame(kf2, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, aplastado);
    agregarKeyFrame(anim, kf2);

    // 1.5s: Ojos abiertos
    KeyFrame* kf3 = crearKeyFrame(1.5f);
    putPoseToKeyFrame(kf3, 6, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    putPoseToKeyFrame(kf3, 7, (Vec3){0,0,0}, (Vec3){0,0,0}, normal);
    agregarKeyFrame(anim, kf3);

    // Segundo parpadeo
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

// Aquí se mueve la cámara, ocurre a la vez que las escenas
void cargarSecuenciaDeComandos() {
    limpiarCola();

    // 1: Acercamiento desde arriba (10 seg)
    Camara cam_inicio = {{0.0, 6.0, 12.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    // 1_Fin: Close up al personaje
    Camara cam_punto_medio = {{0.0, 1.2, 3.0}, {0.0, 0.6, 0.0}, {0.0, 1.0, 0.0}};
    
    encolarEscena(escena_parte1_acercamiento(), cam_inicio, cam_punto_medio, 0);
    encolarEscena(escena_parte2_parpadeo(), cam_punto_medio, cam_punto_medio, 1);
}

void construirEscenario() {
    iniciarPilaHistorial();
    iniciarLista(&lista_objetos_activos);
    iniciarLista(&lista_objetos_estaticos);
    
    escena_raiz = crearNodoEscena(0, (Vec3){0,0,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    NodoEscena *nodo_personaje = crearNodoEscena(2, (Vec3){0,0.5,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    NodoEscena *nodo_piso = crearNodoEscena(1, (Vec3){0,-1.18,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    NodoEscena *nodo_paredes = crearNodoEscena(1, (Vec3){0,0,0}, (Vec3){0,0,0}, (Vec3){1,1,1});
    
    // Z = 2.5 para que se vean las cajas
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
    printf("\n=== INICIALIZANDO SISTEMA ===\n");
    iniciarCola();
    construirEscenario();
    cargarSecuenciaDeComandos();
    
    /* Forzamos la cámara a tomar la posición de la primera escena ya,
    para que no inicie en 0,0,0 y no haya problemas. */
    if (cola_global.frente != NULL) {
        camara_escena = cola_global.frente->accion.mov_camara_inicio;
    }
}

void reiniciarAnimacion() {
    tiempoActual = 0.0f;
    tiempoGlobal = 0.0f;
    hay_accion = 0;

    while (pila_historial.tope != NULL) {
        NodoPilaHistorial *n = popHistorial(); free(n);
    }
    cargarSecuenciaDeComandos(); 
    
    // Reiniciar también la cámara visual
    if (cola_global.frente != NULL) {
        camara_escena = cola_global.frente->accion.mov_camara_inicio;
    }
}

float obtenerTiempoActual() {
    return tiempoGlobal;
}