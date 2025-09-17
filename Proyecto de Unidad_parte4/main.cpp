#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <stack>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

const int ANCHO_VENTANA = 800;
const int ALTO_VENTANA = 600;

// Tipos de herramientas disponibles
enum Herramienta {
    HERRAMIENTA_LINEA_DIRECTA,
    HERRAMIENTA_LINEA_DDA,
    HERRAMIENTA_CIRCULO_PUNTO_MEDIO,
    HERRAMIENTA_ELIPSE_PUNTO_MEDIO,
    HERRAMIENTA_NINGUNA
};

// Estructura para representar un color RGB
struct ColorRGB {
    float r, g, b;
};

// Estructura para representar figuras geométricas
struct Figura {
    Herramienta tipoHerramienta;
    // Lineas: puntos inicio-fin
    int xInicio, yInicio, xFin, yFin;
    // Circulo: centro y radio
    int centroX, centroY, radio;
    // Elipse: radios horizontal y vertical
    int radioX, radioY;
    ColorRGB color;
    int grosor;
};

vector<Figura> figuras;
stack<vector<Figura>> pilaUndo;
stack<vector<Figura>> pilaRedo;

// Estado actual
Herramienta herramientaActual = HERRAMIENTA_LINEA_DIRECTA;
ColorRGB colorActual = {0.f, 0.f, 0.f};
int grosorActual = 1;
bool mostrarCuadricula = true;
bool mostrarEjes = true;
bool esperandoSegundoClic = false;
int primerX = 0;
int primerY = 0;
int anchoVista = ANCHO_VENTANA;
int altoVista = ALTO_VENTANA;

inline int redondearAEntero(float v) {
    return (int) floor(v + 0.5f);
}

// Gest de Undo/Redo
void apilarUndo() {
    pilaUndo.push(figuras);
    while (!pilaRedo.empty()) pilaRedo.pop();
}

void deshacer() {
    if (!pilaUndo.empty()) {
        pilaRedo.push(figuras);
        figuras = pilaUndo.top();
        pilaUndo.pop();
        glutPostRedisplay();
    }
}

void rehacer() {
    if (!pilaRedo.empty()) {
        pilaUndo.push(figuras);
        figuras = pilaRedo.top();
        pilaRedo.pop();
        glutPostRedisplay();
    }
}

// Func para dibujar un punto en (x, y)
void dibujarPunto(int x, int y) {
    glVertex2i(x, y);
}

// Algoritmos de linea directa y DDA
void dibujarLineaDirecta(int x0, int y0, int x1, int y1, int grosor) {
    int dx = x1 - x0, dy = y1 - y0;
    glPointSize(grosor);
    glBegin(GL_POINTS);
    if (dx == 0) {
        int pasoY = (y1 > y0) ? 1 : -1;
        for (int y = y0; y != y1 + pasoY; y += pasoY) dibujarPunto(x0, y);
    } else if (dy == 0) {
        int pasoX = (x1 > x0) ? 1 : -1;
        for (int x = x0; x != x1 + pasoX; x += pasoX) dibujarPunto(x, y0);
    } else {
        float m = (float) dy / dx;
        if (fabs(m) <= 1) {
            int pasoX = (x1 > x0) ? 1 : -1;
            for (int x = x0; x != x1 + pasoX; x += pasoX) dibujarPunto(x, redondearAEntero(m*(x - x0) + y0));
        } else {
            int pasoY = (y1 > y0) ? 1 : -1;
            float invM = (float) dx / dy;
            for (int y = y0; y != y1 + pasoY; y += pasoY) dibujarPunto(redondearAEntero(invM*(y - y0) + x0), y);
        }
    }
    glEnd();
}

void dibujarLineaDDA(int x0, int y0, int x1, int y1, int grosor) {
    int dx = x1 - x0, dy = y1 - y0;
    int pasos = max(abs(dx), abs(dy));
    float x = x0, y = y0;
    float incX = dx / (float) pasos;
    float incY = dy / (float) pasos;
    glPointSize(grosor);
    glBegin(GL_POINTS);
    for (int i = 0; i <= pasos; i++) {
        dibujarPunto(redondearAEntero(x), redondearAEntero(y));
        x += incX;
        y += incY;
    }
    glEnd();
}

// Algoritmo de Círculo y Elipse con el método del punto medio
void dibujarPuntosCirculo(int cx, int cy, int x, int y) {
    dibujarPunto(cx + x, cy + y);
    dibujarPunto(cx - x, cy + y);
    dibujarPunto(cx + x, cy - y);
    dibujarPunto(cx - x, cy - y);
    dibujarPunto(cx + y, cy + x);
    dibujarPunto(cx - y, cy + x);
    dibujarPunto(cx + y, cy - x);
    dibujarPunto(cx - y, cy - x);
}

void dibujarCirculoPuntoMedio(int cx, int cy, int r, int grosor) {
    int x = 0, y = r;
    int p = 1 - r;
    glPointSize(grosor);
    glBegin(GL_POINTS);
    dibujarPuntosCirculo(cx, cy, x, y);
    while (x < y) {
        x++;
        if (p < 0) p += 2*x + 1;
        else {
            y--;
            p += 2*(x - y) + 1;
        }
        dibujarPuntosCirculo(cx, cy, x, y);
    }
    glEnd();
}

void dibujarPuntosElipse(int cx, int cy, int x, int y) {
    dibujarPunto(cx + x, cy + y);
    dibujarPunto(cx - x, cy + y);
    dibujarPunto(cx + x, cy - y);
    dibujarPunto(cx - x, cy - y);
}

void dibujarElipsePuntoMedio(int cx, int cy, int rx, int ry, int grosor) {
    int x = 0, y = ry;
    long rx2 = rx*rx, ry2 = ry*ry;
    long dos_rx2 = 2*rx2, dos_ry2 = 2*ry2;
    double p1 = ry2 - rx2 * ry + 0.25*rx2;
    glPointSize(grosor);
    glBegin(GL_POINTS);
    while (dos_ry2*x <= dos_rx2*y) {
        dibujarPuntosElipse(cx, cy, x, y);
        if (p1 < 0) {
            x++;
            p1 += dos_ry2*x + ry2;
        } else {
            x++;
            y--;
            p1 += dos_ry2*x - dos_rx2*y + ry2;
        }
    }
    double p2 = ry2*(x+0.5)*(x+0.5) + rx2*(y-1)*(y-1) - rx2*ry2;
    while (y >= 0) {
        dibujarPuntosElipse(cx, cy, x, y);
        if (p2 > 0) {
            y--;
            p2 -= dos_rx2*y + rx2;
        } else {
            y--;
            x++;
            p2 += dos_ry2*x - dos_rx2*y + rx2;
        }
    }
    glEnd();
}

// Func para dibujar la figura completa
void dibujarFigura(const Figura &f) {
    glColor3f(f.color.r, f.color.g, f.color.b);
    switch (f.tipoHerramienta) {
        case HERRAMIENTA_LINEA_DIRECTA:
            dibujarLineaDirecta(f.xInicio, f.yInicio, f.xFin, f.yFin, f.grosor);
            break;
        case HERRAMIENTA_LINEA_DDA:
            dibujarLineaDDA(f.xInicio, f.yInicio, f.xFin, f.yFin, f.grosor);
            break;
        case HERRAMIENTA_CIRCULO_PUNTO_MEDIO:
            dibujarCirculoPuntoMedio(f.centroX, f.centroY, f.radio, f.grosor);
            break;
        case HERRAMIENTA_ELIPSE_PUNTO_MEDIO:
            dibujarElipsePuntoMedio(f.centroX, f.centroY, f.radioX, f.radioY, f.grosor);
            break;
        default:
            break;
    }
}

// Func para redibujar todo el contenido
void redibujar
