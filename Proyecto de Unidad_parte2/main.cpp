#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>
using namespace std;

const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;

// Herramientas disponibles
enum DrawingTool {
    TOOL_DIRECT,
    TOOL_DDA,
    TOOL_NONE
};

// Estructura para colores RGB
struct RGBColor {
    float r, g, b;
};

// Estructura para líneas con propiedades
struct LineSegment {
    DrawingTool toolType;
    int startX, startY;
    int endX, endY;
    RGBColor color;
    int thickness;
};

vector<LineSegment> lineCollection;  // Colección de líneas dibujadas

// Estado actual de dibujo
DrawingTool selectedTool = TOOL_DIRECT;
RGBColor selectedColor = {0.f, 0.f, 0.f};  // Negro
int selectedThickness = 1;

bool firstClickPending = false;
int firstClickX = 0, firstClickY = 0;

inline int roundFloatToInt(float val) {
    return static_cast<int>(floor(val + 0.5f));
}

// Dibuja un solo punto en (x,y)
void drawPoint(int x, int y) {
    glVertex2i(x, y);
}

// Algoritmo línea directa
void drawDirectLine(int x0, int y0, int x1, int y1, int thickness) {
    int deltaX = x1 - x0;
    int deltaY = y1 - y0;

    glPointSize(thickness);
    glBegin(GL_POINTS);

    if (deltaX == 0) {
        int step = (y1 > y0) ? 1 : -1;
        for (int y = y0; y != y1 + step; y += step) {
            drawPoint(x0, y);
        }
    } else if (deltaY == 0) {
        int step = (x1 > x0) ? 1 : -1;
        for (int x = x0; x != x1 + step; x += step) {
            drawPoint(x, y0);
        }
    } else {
        float slope = (float)deltaY / deltaX;
        if (fabs(slope) <= 1) {
            int step = (x1 > x0) ? 1 : -1;
            for (int x = x0; x != x1 + step; x += step) {
                drawPoint(x, roundFloatToInt(slope * (x - x0) + y0));
            }
        } else {
            int step = (y1 > y0) ? 1 : -1;
            float inverseSlope = (float)deltaX / deltaY;
            for (int y = y0; y != y1 + step; y += step) {
                drawPoint(roundFloatToInt(inverseSlope * (y - y0) + x0), y);
            }
        }
    }

    glEnd();
}

// Algoritmo DDA para línea
void drawDDALine(int x0, int y0, int x1, int y1, int thickness) {
    int diffX = x1 - x0;
    int diffY = y1 - y0;

    int steps = max(abs(diffX), abs(diffY));
    float x = x0;
    float y = y0;
    float incX = diffX / (float)steps;
    float incY = diffY / (float)steps;

    glPointSize(thickness);
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; ++i) {
        drawPoint(roundFloatToInt(x), roundFloatToInt(y));
        x += incX;
        y += incY;
    }
    glEnd();
}

// Función para dibujar una línea según su tipo
void renderLine(const LineSegment &line) {
    glColor3f(line.color.r, line.color.g, line.color.b);
    if (line.toolType == TOOL_DIRECT) {
        drawDirectLine(line.startX, line.startY, line.endX, line.endY, line.thickness);
    } else if (line.toolType == TOOL_DDA) {
        drawDDALine(line.startX, line.startY, line.endX, line.endY, line.thickness);
    }
}

// Redibuja todas las líneas almacenadas
void refreshCanvas() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (const auto& line : lineCollection) {
        renderLine(line);
    }
    glutSwapBuffers();
}

// Callbacks GLUT
void onDisplay() {
    refreshCanvas();
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
    int adjustedY = WIN_HEIGHT - y;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (!firstClickPending) {
            firstClickX = x;
            firstClickY = adjustedY;
            firstClickPending = true;
        } else {
            LineSegment newLine;
            newLine.toolType = selectedTool;
            newLine.color = selectedColor;
            newLine.thickness = selectedThickness;
            newLine.startX = firstClickX;
            newLine.startY = firstClickY;
            newLine.endX = x;
            newLine.endY = adjustedY;

            lineCollection.push_back(newLine);
            firstClickPending = false;

            glutPostRedisplay();
        }
    }
}

// Menú para selección de herramienta y color
void menuHandler(int choice) {
    switch (choice) {
        case 1: selectedTool = TOOL_DIRECT; break;
        case 2: selectedTool = TOOL_DDA; break;
        case 10: selectedColor = {0.f, 0.f, 0.f}; break;    // Negro
        case 11: selectedColor = {1.f, 0.f, 0.f}; break;    // Rojo
        case 12: selectedColor = {0.f, 0.f, 1.f}; break;    // Azul
    }
    glutPostRedisplay();
}

void setupMenus() {
    int algorithmMenu = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Linea Directa", 1);
    glutAddMenuEntry("Linea DDA", 2);

    int colorMenu = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Negro", 10);
    glutAddMenuEntry("Rojo", 11);
    glutAddMenuEntry("Azul", 12);

    int mainMenu = glutCreateMenu(menuHandler);
    glutAddSubMenu("Algoritmo", algorithmMenu);
    glutAddSubMenu("Color", colorMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void initGraphics() {
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glPointSize(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_WIDTH, 0, WIN_HEIGHT);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutCreateWindow("Version Mejorada - DDA y Directa");

    initGraphics();
    setupMenus();

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutMouseFunc(onMouseClick);

    glutMainLoop();
    return 0;
}

