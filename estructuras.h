#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    float ambient[4];   // Color bajo, luz ambiental
    float diffuse[4];   // Color bajo, luz directa (el color principal del objeto)
    float specular[4];  // Color del brillo
    float shininess;    // Qué tan concentrado es el brillo (0-128)
} Material;

// Vector 3D
typedef struct {
    float x, y, z;
} Vec3;

// Árbol que gestionará la escena
typedef struct NodoEscena {
    int id;
    int tipo;   // 0: Vacío, 0: Geometría, 2: Personaje

    Vec3 pos;
    Vec3 rot;
    Vec3 esc;

    void *dato;
    bool visible;

    struct NodoEscena *padre;
    struct NodoEscena **hijos;
    int num_hijos;
} NodoEscena;

// En estructuras.h
typedef struct Joint {
    int id;
    char nombre[32];

    // Transformaciones
    Vec3 pos_local;
    Vec3 rot_local;
    Vec3 esc_local;       

    Vec3 pos_actual;
    Vec3 rot_actual;
    Vec3 esc_actual;

    // Materiales y gráficos
    Material material;
    int textura_id;
    void (*drawFunc)(void);

    struct Joint *padre;      
    struct Joint *hijo[10];
    int num_hijos;
} Joint;

// Control de la vista
typedef struct {
    Vec3 eye;       // Dónde está la cámara
    Vec3 center;    // Hacia dónde mira
    Vec3 up;        // Qué vector es "arriba"
} Camara;

// Pose individual para un joint en un momento dado
typedef struct {
    int joint_id;
    Vec3 pos;
    Vec3 rot;
} JointTransform;

// Representa un momento en el tiempo
typedef struct KeyFrame {
    float timeStamp;        // Tiempo en segundos
    
    JointTransform *poses;
    int num_poses;

    struct KeyFrame *sgt;  // Puntero al siguiente KeyFrame (Lista Enlazada)
} KeyFrame;

// Contenedor de la animación completa
typedef struct {
    char nombre[32];
    float duracion;     // Duración total en segundos
    KeyFrame *frames;   // Inicio de la lista
} Animacion;

// Orden de dibujo simple
typedef struct {
    int tipo;           // Tipo de primitiva
    Vec3 pos_global;    // Posición final calculada
    Vec3 rot_global;    // Rotación final calculada
    Vec3 esc_global;    // Escala final calculada

    // Apariencia
    int textura_id;
    Material material;   // Para configurar glMaterialfv
} RenderCommand;

typedef struct NodoPila {
    RenderCommand cmd;
    struct NodoPila *sgt;
} NodoPila;

// La Pila de Renderizado
typedef struct {
    NodoPila *tope;
    int cont;
} PilaRender;

typedef struct {
    int accion_tipo;
    Animacion *anim_actual;
    Camara mov_camara;
    float duracion;
} AccionEscena;

typedef struct NodoCola {
    AccionEscena accion;
    struct NodoCola *sgt;
} NodoCola;

// La Cola de Acciones
typedef struct {
    NodoCola *frente;
    NodoCola *fondo;
} ColaAcciones;

#endif