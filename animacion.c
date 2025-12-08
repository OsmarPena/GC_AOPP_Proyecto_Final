#include "estructuras.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

ColaAcciones cola_global;
AccionEscena accion_act; 
int hay_accion = 0;   
Camara camara_escena;

// Variables internas
static float tiempoActual = 0.0f;
static float velocidadAnimacion = 1.0f;
static Animacion* animacion_principal = NULL;
static int animacionCompletada = 0;

// Variables para interpolación de cámara
static Camara cam_inic;
static Camara camEnd;

KeyFrame* crearKeyFrame(float tiempo) {
    KeyFrame* kf = (KeyFrame*)malloc(sizeof(KeyFrame));
    kf->timeStamp = tiempo;
    kf->poses = NULL;
    kf->num_poses = 0;
    kf->sgt = NULL;
    return kf;
}

void putPoseToKeyFrame(KeyFrame* kf, int joint_id, Vec3 pos, Vec3 rot) {
    kf->num_poses++;
    kf->poses = (JointTransform*)realloc(kf->poses, sizeof(JointTransform) * kf->num_poses);
    int id = kf->num_poses - 1;
    kf->poses[id].joint_id = joint_id;
    kf->poses[id].pos = pos;
    kf->poses[id].rot = rot;
}

void agregarKeyFrame(Animacion* anim, KeyFrame* kf) {
    if (anim->frames == NULL) {
        anim->frames = kf;
    } else {
        KeyFrame* temp = anim->frames;
        while (temp->sgt != NULL) temp = temp->sgt;
        temp->sgt = kf;
    }
}

Vec3 interpolarVec3(Vec3 a, Vec3 b, float t) {
    Vec3 resultado;
    resultado.x = a.x + (b.x - a.x) * t;
    resultado.y = a.y + (b.y - a.y) * t;
    resultado.z = a.z + (b.z - a.z) * t;
    return resultado;
}

void aplicarInterpolacion(Joint* robot, int joint_id, Vec3 pos, Vec3 rot) {
    if (robot == NULL) return;
    if (robot->id == joint_id) {
        robot->pos_actual = pos;
        robot->rot_actual = rot;
        return;
    }
    for (int i = 0; i < robot->num_hijos; i++) {
        aplicarInterpolacion(robot->hijo[i], joint_id, pos, rot);
    }
}

void getKeyFramesAdyacentes(Animacion* anim, float tiempo, KeyFrame** ant, KeyFrame** sgt) {
    *ant = NULL; *sgt = NULL;
    if (anim == NULL || anim->frames == NULL) return;
    KeyFrame* kf = anim->frames;
    while (kf != NULL) {
        if (kf->timeStamp <= tiempo) *ant = kf;
        if (kf->timeStamp > tiempo && *sgt == NULL) { *sgt = kf; break; }
        kf = kf->sgt;
    }
    if (*sgt == NULL && *ant != NULL) *sgt = *ant;
    if (*ant == NULL) *ant = anim->frames;
}

void iniciarCola() {
    cola_global.frente = NULL;
    cola_global.fondo = NULL;
    hay_accion = 0;
}

void encolarEscena(Animacion* anim, Camara cam_inicial, Camara cam_final) {
    NodoCola* nuevo = (NodoCola*)malloc(sizeof(NodoCola));
    nuevo->accion.anim_actual = anim;
    nuevo->accion.mov_camara = cam_final;
    nuevo->accion.duracion = anim->duracion;
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
            
            // Lógica de cámara
            if (!hay_accion) cam_inic = (Camara){{-5.0, 2.0, 12.0}, {-5.0, 1.0, 0.0}, {0,1,0}};
            else cam_inic = camara_escena;
            camEnd = accion_act.mov_camara;
            
            tiempoActual = 0.0f;
            hay_accion = 1;
            cola_global.frente = temp->sgt;
            
            if (cola_global.frente == NULL) cola_global.fondo = NULL;
            free(temp);
        } else {
            // Loop infinito de la última escena si se acaba la cola
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
    if (tiempoActual > animacion_principal->duracion)
        tiempoActual = animacion_principal->duracion;
    
    KeyFrame *ant, *sgt;
    getKeyFramesAdyacentes(animacion_principal, tiempoActual, &ant, &sgt);
    if (ant == NULL || sgt == NULL)
        return;
    
    float t = 0.0f;
    
    if (sgt->timeStamp != ant->timeStamp) 
        t = (tiempoActual - ant->timeStamp) / (sgt->timeStamp - ant->timeStamp);
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;
    
    for (int i = 0; i < ant->num_poses; i++) {
        JointTransform poseAnt = ant->poses[i];
        JointTransform poseSig = poseAnt; 
        for (int j = 0; j < sgt->num_poses; j++) {
            if (sgt->poses[j].joint_id == poseAnt.joint_id) {
                poseSig = sgt->poses[j];
                break;
            }
        }

        Vec3 posInterpolada = interpolarVec3(poseAnt.pos, poseSig.pos, t);
        Vec3 rotInterpolada = interpolarVec3(poseAnt.rot, poseSig.rot, t);
        aplicarInterpolacion(robot, poseAnt.joint_id, posInterpolada, rotInterpolada);
    }
}

Animacion* escena_prueba() {
    Animacion* anim = (Animacion*)malloc(sizeof(Animacion));
    strcpy(anim->nombre, "Prueba - Giro de Cabeza");
    anim->duracion = 10.0f; 
    anim->frames = NULL;

    // Neutro
    KeyFrame* kf0 = crearKeyFrame(0.0f);
    putPoseToKeyFrame(kf0, 0, (Vec3){0.0, 0.5, 0.0}, (Vec3){0, 0, 0});
    putPoseToKeyFrame(kf0, 1, (Vec3){0, 0, 0}, (Vec3){0, 0, 0});
    agregarKeyFrame(anim, kf0);
    
    // Izquierda
    KeyFrame* kf25 = crearKeyFrame(2.5f);
    putPoseToKeyFrame(kf25, 0, (Vec3){0.0, 0.5, 0.0}, (Vec3){0, 0, 0});
    putPoseToKeyFrame(kf25, 1, (Vec3){0, 0, 0}, (Vec3){0, -45, 0});
    agregarKeyFrame(anim, kf25);
    
    // Centro
    KeyFrame* kf5 = crearKeyFrame(5.0f);
    putPoseToKeyFrame(kf5, 0, (Vec3){0.0, 0.5, 0.0}, (Vec3){0, 0, 0});
    putPoseToKeyFrame(kf5, 1, (Vec3){0, 0, 0}, (Vec3){0, 0, 0});
    agregarKeyFrame(anim, kf5);
    
    // Derecha
    KeyFrame* kf75 = crearKeyFrame(7.5f);
    putPoseToKeyFrame(kf75, 0, (Vec3){0.0, 0.5, 0.0}, (Vec3){0, 0, 0});
    putPoseToKeyFrame(kf75, 1, (Vec3){0, 0, 0}, (Vec3){0, 45, 0});
    agregarKeyFrame(anim, kf75);
    
    // Centro
    KeyFrame* kf10 = crearKeyFrame(10.0f);
    putPoseToKeyFrame(kf10, 0, (Vec3){0.0, 0.5, 0.0}, (Vec3){0, 0, 0});
    putPoseToKeyFrame(kf10, 1, (Vec3){0, 0, 0}, (Vec3){0, 0, 0});
    agregarKeyFrame(anim, kf10);
    
    return anim;
}

// Inicio de toda la animación
void iniciarGuion() {
    iniciarCola();

    Camara camFija = {{0.0, 2.0, 8.0},
                      {0.0, 1.0, 0.0},
                      {0.0, 1.0, 0.0}};
    
    encolarEscena(escena_prueba(), camFija, camFija);
}

// Funciones extra del main
void reiniciarAnimacion() {
    iniciarGuion(); }
float obtenerTiempoActual() {
    return tiempoActual; }