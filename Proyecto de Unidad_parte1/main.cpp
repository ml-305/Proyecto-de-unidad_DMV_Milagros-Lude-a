#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>
using namespace std;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Representa un color RGB
struct RGB {
    float red, green, blue;
};

// Representa una línea con inicio, fin, color y grosor
struct Line {
    int startX, startY;
    int endX, endY;
    RGB color;
    int thickness;
};

vector<Line> lines; // almacena todas las líneas dibujadas

// Estado actual con color negro y grosor 1
RGB activeColor = {0.f, 0.f, 0.f};
int activeThickness = 1;

bool waitingForSecondClick = false;
int xFirstClick = 0, yFirstClick = 0;

inline int roundToInt(float v) {
    return static_cast<int>(floor(v + 0.5f));
}

// Pinta un pixel en coordenadas (x,y)
void drawPixel(int x, int y) {
    glVertex2i(x, y);
}

// Algoritmo de línea directa dibujando puntos
void drawLineDirect(int x0, int y0, int x1, int y1, int thickness) {
    int deltaX = x1 - x0;
    int deltaY = y1 - y0;

    glPointSize(thickness);
    glBegin(GL_POINTS);

    if (deltaX == 0) {
        int step = (y1 > y0) ? 1 : -1;
        for (int y = y0; y != y1 + step; y += step) {
            drawPixel(x0, y);
        }
    } else if (deltaY == 0) {
        int step = (x1 > x0) ? 1 : -1;
        for (int x = x0; x != x1 + step; x += step) {
            drawPixel(x, y0);
        }
    } else {
        float slope = (float)deltaY / deltaX;
        if (fabs(slope) <= 1) {
            int step = (x1 > x0) ? 1 : -1;
            for (int x = x0; x != x1 + step; x += step) {
                drawPixel(x, roundToInt(slope * (x - x0) + y0));
            }
        } else {
            int step = (y1 > y0) ? 1 : -1;
            float inverseSlope = (float)deltaX / deltaY;
            for (int y = y0; y != y1 + step; y += step) {
                drawPixel(roundToInt(inverseSlope * (y - y0) + x0), y);
            }
        }
    }

    glEnd();
}

// Redibuja todas las líneas almacenadas
void redrawScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& line : lines) {
        glColor3f(line.color.red, line.color.green, line.color.blue);
        drawLineDirect(line.startX, line.startY, line.endX, line.endY, line.thickness);
    }

    glutSwapBuffers();
}

// Callbacks GLUT
void onDisplay() {
    redrawScene();
}

void onReshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void onMouseClick(int button, int state, int x, int y) {
    int correctedY = WINDOW_HEIGHT - y;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (!waitingForSecondClick) {
            xFirstClick = x;
            yFirstClick = correctedY;
            waitingForSecondClick = true;
        } else {
            Line newLine;
            newLine.color = activeColor;
            newLine.thickness = activeThickness;
            newLine.startX = xFirstClick;
            newLine.startY = yFirstClick;
            newLine.endX = x;
            newLine.endY = correctedY;

            lines.push_back(newLine);
            waitingForSecondClick = false;

            glutPostRedisplay();
        }
    }
}

// Inicialización OpenGL
void initializeOpenGL() {
    glClearColor(1.f, 1.f, 1.f, 1.f); // fondo blanco
    glPointSize(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Líneas con Algoritmo Directo");

    initializeOpenGL();

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutMouseFunc(onMouseClick);

    glutMainLoop();
    return 0;
}

