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

enum Herramienta {
    HERRAMIENTA_LINEA_DIRECTA,
    HERRAMIENTA_LINEA_DDA,
    HERRAMIENTA_CIRCULO_PUNTO_MEDIO,
    HERRAMIENTA_ELIPSE_PUNTO_MEDIO,
    HERRAMIENTA_NINGUNA
};


struct ColorRGB {
    float r, g, b;
};

struct Figura {
    Herramienta tipoHerramienta;
    // Líneas: puntos inicio-fin
    int xInicio, yInicio, xFin, yFin;
    // Círculo: centro y radio
    int centroX, centroY, radio;
    // Elipse: radios horizontal y vertical
    int radioX, radioY;
    ColorRGB color;
    int grosor;
};

vector<Figura> figuras;
stack<vector<Figura>> pilaDeshacer;
stack<vector<Figura>> pilaRehacer;

Herramienta herramientaActual = HERRAMIENTA_LINEA_DIRECTA;
ColorRGB colorActual = {0.f, 0.f, 0.f};
int grosorActual = 1;
bool mostrarCuadricula = true;
bool mostrarEjes = true;
bool esperandoSegundoClick = false;
int primerX = 0;
int primerY = 0;
int anchoViewport = ANCHO_VENTANA;
int altoViewport = ALTO_VENTANA;

inline int redondearAEntero(float v) {
    return (int) floor(v + 0.5f);
}

// Gestión Deshacer/Rehacer
void guardarParaDeshacer() {
    pilaDeshacer.push(figuras);
    while (!pilaRehacer.empty()) pilaRehacer.pop();
}

void deshacer() {
    if (!pilaDeshacer.empty()) {
        pilaRehacer.push(figuras);
        figuras = pilaDeshacer.top();
        pilaDeshacer.pop();
        glutPostRedisplay();
    }
}

void rehacer() {
    if (!pilaRehacer.empty()) {
        pilaDeshacer.push(figuras);
        figuras = pilaRehacer.top();
        pilaRehacer.pop();
        glutPostRedisplay();
    }
}

void dibujarPunto(int x, int y) {
    glVertex2i(x, y);
}

void dibujarLineaDirecta(int x0, int y0, int x1, int y1, int grosor) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int pasos = max(abs(dx), abs(dy));

    float xInc = dx / (float) pasos;
    float yInc = dy / (float) pasos;

    float x = x0;
    float y = y0;

    glPointSize(grosor);
    glBegin(GL_POINTS);
    for (int i = 0; i <= pasos; i++) {
        dibujarPunto(redondearAEntero(x), redondearAEntero(y));
        x += xInc;
        y += yInc;
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

void redibujarTodo() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Dibujar cuadrícula
    if (mostrarCuadricula) {
        glColor3f(0.85f, 0.85f, 0.85f);
        glBegin(GL_LINES);
        for (int x = 0; x <= anchoViewport; x += 20) {
            glVertex2i(x, 0);
            glVertex2i(x, altoViewport);
        }
        for (int y = 0; y <= altoViewport; y += 20) {
            glVertex2i(0, y);
            glVertex2i(anchoViewport, y);
        }
        glEnd();
    }

    // Dibujar ejes
    if (mostrarEjes) {
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_LINES);
        glVertex2i(0, altoViewport / 2);
        glVertex2i(anchoViewport, altoViewport / 2);
        glVertex2i(anchoViewport / 2, 0);
        glVertex2i(anchoViewport / 2, altoViewport);
        glEnd();
    }

    for (auto &fig : figuras) {
        dibujarFigura(fig);
    }
    glutSwapBuffers();
}


void mostrar() {
    redibujarTodo();
}

void reajustar(int w, int h) {
    anchoViewport = w;
    altoViewport = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void raton(int boton, int estado, int x, int y) {
    int ox = x;
    int oy = altoViewport - y;
    if (boton == GLUT_LEFT_BUTTON && estado == GLUT_DOWN) {
        if (!esperandoSegundoClick) {
            primerX = ox;
            primerY = oy;
            esperandoSegundoClick = true;
        } else {
            guardarParaDeshacer();
            Figura f;
            f.color = colorActual;
            f.grosor = grosorActual;
            switch (herramientaActual) {
                case HERRAMIENTA_LINEA_DIRECTA:
                case HERRAMIENTA_LINEA_DDA:
                    f.tipoHerramienta = herramientaActual;
                    f.xInicio = primerX;
                    f.yInicio = primerY;
                    f.xFin = ox;
                    f.yFin = oy;
                    break;
                case HERRAMIENTA_CIRCULO_PUNTO_MEDIO:
                    f.tipoHerramienta = HERRAMIENTA_CIRCULO_PUNTO_MEDIO;
                    f.centroX = primerX;
                    f.centroY = primerY;
                    f.radio = round(sqrt(pow(ox - primerX, 2) + pow(oy - primerY, 2)));
                    break;
                case HERRAMIENTA_ELIPSE_PUNTO_MEDIO:
                    f.tipoHerramienta = HERRAMIENTA_ELIPSE_PUNTO_MEDIO;
                    f.centroX = primerX;
                    f.centroY = primerY;
                    f.radioX = abs(ox - primerX);
                    f.radioY = abs(oy - primerY);
                    break;
                default:
                    break;
            }
            figuras.push_back(f);
            esperandoSegundoClick = false;
            glutPostRedisplay();
        }
    }
}

void manejarMenu(int opcion) {
    switch (opcion) {
        case 1: herramientaActual = HERRAMIENTA_LINEA_DIRECTA; break;
        case 2: herramientaActual = HERRAMIENTA_LINEA_DDA; break;
        case 3: herramientaActual = HERRAMIENTA_CIRCULO_PUNTO_MEDIO; break;
        case 4: herramientaActual = HERRAMIENTA_ELIPSE_PUNTO_MEDIO; break;
        case 10: colorActual = {0.f, 0.f, 0.f}; break;      // Negro
        case 11: colorActual = {1.f, 0.f, 0.f}; break;      // Rojo
        case 12: colorActual = {0.f, 1.f, 0.f}; break;      // Verde
        case 13: colorActual = {0.f, 0.f, 1.f}; break;      // Azul
        case 20: grosorActual = 1; break;
        case 21: grosorActual = 2; break;
        case 22: grosorActual = 3; break;
        case 23: grosorActual = 5; break;
        case 30: mostrarCuadricula = !mostrarCuadricula; break;
        case 31: mostrarEjes = !mostrarEjes; break;
        case 40: guardarParaDeshacer(); figuras.clear(); break;
        case 41: deshacer(); break;
        case 42: rehacer(); break;

    }
    glutPostRedisplay();
}

void crearMenus() {
    int menuDibujo = glutCreateMenu(manejarMenu);
    glutAddMenuEntry("Línea Directa", 1);
    glutAddMenuEntry("Línea DDA", 2);
    glutAddMenuEntry("Círculo PM", 3);
    glutAddMenuEntry("Elipse PM", 4);

    int menuColor = glutCreateMenu(manejarMenu);
    glutAddMenuEntry("Negro", 10);
    glutAddMenuEntry("Rojo", 11);
    glutAddMenuEntry("Verde", 12);
    glutAddMenuEntry("Azul", 13);

    int menuGrosor = glutCreateMenu(manejarMenu);
    glutAddMenuEntry("1px", 20);
    glutAddMenuEntry("2px", 21);
    glutAddMenuEntry("3px", 22);
    glutAddMenuEntry("5px", 23);

    int menuVista = glutCreateMenu(manejarMenu);
    glutAddMenuEntry("Mostrar/Ocultar Cuadrícula", 30);
    glutAddMenuEntry("Mostrar/Ocultar Ejes", 31);

    int menuHerramientas = glutCreateMenu(manejarMenu);
    glutAddMenuEntry("Limpiar", 40);
    glutAddMenuEntry("Deshacer", 41);
    glutAddMenuEntry("Rehacer", 42);
    glutAddMenuEntry("Exportar PPM", 43);

    int menuPrincipal = glutCreateMenu(manejarMenu);
    glutAddSubMenu("Dibujo", menuDibujo);
    glutAddSubMenu("Color", menuColor);
    glutAddSubMenu("Grosor", menuGrosor);
    glutAddSubMenu("Vista", menuVista);
    glutAddSubMenu("Herramientas", menuHerramientas);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void inicializarGL() {
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glPointSize(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, ANCHO_VENTANA, 0, ALTO_VENTANA);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(ANCHO_VENTANA, ALTO_VENTANA);
    glutCreateWindow("Mini CAD Raster");
    inicializarGL();
    crearMenus();
    glutDisplayFunc(mostrar);
    glutReshapeFunc(reajustar);
    glutMouseFunc(raton);
    glutMainLoop();
    return 0;
}

