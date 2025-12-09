#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Vector 3D
typedef struct {
    float x, y, z;
} Vec3;

// Materiales
typedef struct {
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float shininess;
} Material;

// Árbol de la escena (Jerarquía Global)
typedef struct NodoEscena {
    int id;
    int tipo;   // 1: Geometría estática (Piso / Cajas), 2: Personaje (Robot)
    Vec3 pos;
    Vec3 rot;
    Vec3 esc;
    
    void *dato; // Puntero genérico (puede apuntar al Robot Joint*)
    bool visible;

    struct NodoEscena *padre;
    struct NodoEscena **hijos;
    int num_hijos;
} NodoEscena;

// Árbol del robot (Jerarquía Interna)
typedef struct Joint {
    int id;
    char nombre[32];
    Vec3 pos_local, rot_local, esc_local;       
    Vec3 pos_actual, rot_actual, esc_actual;
    Material material;
    int textura_id;
    void (*drawFunc)(void);
    struct Joint *padre;      
    struct Joint *hijo[10];
    int num_hijos;
} Joint;

// Lista enlazada (Gestión de objetos)
typedef struct NodoListaObjeto {
    NodoEscena *objeto;
    struct NodoListaObjeto *sgt;
} NodoListaObjeto;

typedef struct {
    NodoListaObjeto *cabeza;
    int count;
} ListaObjetos;

// Pila del historial
typedef struct NodoPilaHistorial {
    Vec3 pos_anterior;
    Vec3 rot_anterior;
    int objeto_id;
    struct NodoPilaHistorial *sgt;
} NodoPilaHistorial;

typedef struct {
    NodoPilaHistorial *tope;
    int cont;
} PilaHistorial;

/* Estructura auxiliar para las
transformadas entre key frames. */
typedef struct {
    int joint_id;
    Vec3 pos, rot, esc;
} JointTransform;

typedef struct KeyFrame {
    float timeStamp;
    JointTransform *poses;
    int num_poses;
    struct KeyFrame *sgt;
} KeyFrame;

typedef struct {
    char nombre[32];
    float duracion;
    KeyFrame *frames;
} Animacion;

typedef struct {
    Vec3 eye, center, up;
} Camara;

typedef struct {
    int accion_tipo;
    Animacion *anim_actual;
    Camara mov_camara_inicio;
    Camara mov_camara_fin;
    float duracion;
    int es_corte;
} AccionEscena;

typedef struct NodoCola {
    AccionEscena accion;
    struct NodoCola *sgt;
} NodoCola;

typedef struct {
    NodoCola *frente;
    NodoCola *fondo;
} ColaAcciones;

#endif