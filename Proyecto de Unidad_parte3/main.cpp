#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>
using namespace std;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Tipos de herramientas de dibujo
enum ToolType {
    TOOL_LINE_DIRECT,
    TOOL_LINE_DDA,
    TOOL_CIRCLE_MIDPOINT,
    TOOL_ELLIPSE_MIDPOINT,
    TOOL_NONE
};

// Representa un color RGB
struct ColorRGB {
    float red, green, blue;
};

// Representa una figura geométrica
struct Figure {
    ToolType tool;
    // Coordenadas para líneas
    int x_start, y_start, x_end, y_end;
    // Para círculo
    int center_x, center_y, radius;
    // Para elipse
    int radius_x, radius_y;
    // Propiedades comunes
    ColorRGB color;
    int thickness;
};

// Lista de figuras dibujadas
vector<Figure> figures;

// Estado actual de dibujo
ToolType currentTool = TOOL_LINE_DIRECT;
ColorRGB currentColor = {0.f, 0.f, 0.f};  // negro
int currentThickness = 1;
bool isWaitingSecondClick = false;
int firstClickX = 0;
int firstClickY = 0;

inline int roundToInt(float v) {
    return static_cast<int>(floor(v + 0.5f));
}

// Pinta un punto en (x,y)
void drawPoint(int x, int y) {
    glVertex2i(x, y);
}

// Algoritmo línea directa
void drawLineDirect(int x0, int y0, int x1, int y1, int thickness) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    glPointSize(thickness);
    glBegin(GL_POINTS);

    if (dx == 0) {
        int stepY = (y1 > y0) ? 1 : -1;
        for (int y = y0; y != y1 + stepY; y += stepY) {
            drawPoint(x0, y);
        }
    } else if (dy == 0) {
        int stepX = (x1 > x0) ? 1 : -1;
        for (int x = x0; x != x1 + stepX; x += stepX) {
            drawPoint(x, y0);
        }
    } else {
        float m = (float) dy / dx;
        if (fabs(m) <= 1) {
            int stepX = (x1 > x0) ? 1 : -1;
            for (int x = x0; x != x1 + stepX; x += stepX) {
                drawPoint(x, roundToInt(m * (x - x0) + y0));
            }
        } else {
            int stepY = (y1 > y0) ? 1 : -1;
            float invM = (float) dx / dy;
            for (int y = y0; y != y1 + stepY; y += stepY) {
                drawPoint(roundToInt(invM * (y - y0) + x0), y);
            }
        }
    }
    glEnd();
}

// Algoritmo línea DDA
void drawLineDDA(int x0, int y0, int x1, int y1, int thickness) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = max(abs(dx), abs(dy));
    float x = x0;
    float y = y0;
    float incX = dx / (float) steps;
    float incY = dy / (float) steps;

    glPointSize(thickness);
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++) {
        drawPoint(roundToInt(x), roundToInt(y));
        x += incX;
        y += incY;
    }
    glEnd();
}

// Dibuja los 8 puntos simétricos de un círculo
void drawCircle8Points(int cx, int cy, int x, int y) {
    drawPoint(cx + x, cy + y);
    drawPoint(cx - x, cy + y);
    drawPoint(cx + x, cy - y);
    drawPoint(cx - x, cy - y);
    drawPoint(cx + y, cy + x);
    drawPoint(cx - y, cy + x);
    drawPoint(cx + y, cy - x);
    drawPoint(cx - y, cy - x);
}

// Algoritmo círculo Punto Medio
void drawCircleMidpoint(int cx, int cy, int radius, int thickness) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;

    glPointSize(thickness);
    glBegin(GL_POINTS);
    drawCircle8Points(cx, cy, x, y);
    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
        drawCircle8Points(cx, cy, x, y);
    }
    glEnd();
}

// Dibuja los 4 puntos simétricos de una elipse
void drawEllipse4Points(int cx, int cy, int x, int y) {
    drawPoint(cx + x, cy + y);
    drawPoint(cx - x, cy + y);
    drawPoint(cx + x, cy - y);
    drawPoint(cx - x, cy - y);
}

// Algoritmo elipse Punto Medio
void drawEllipseMidpoint(int cx, int cy, int rx, int ry, int thickness) {
    int x = 0;
    int y = ry;
    long rx2 = rx * rx;
    long ry2 = ry * ry;
    long two_rx2 = 2 * rx2;
    long two_ry2 = 2 * ry2;
    double p1 = ry2 - rx2 * ry + 0.25 * rx2;

    glPointSize(thickness);
    glBegin(GL_POINTS);

    // Región 1
    while ((two_ry2 * x) <= (two_rx2 * y)) {
        drawEllipse4Points(cx, cy, x, y);
        if (p1 < 0) {
            x++;
            p1 += two_ry2 * x + ry2;
        } else {
            x++;
            y--;
            p1 += two_ry2 * x - two_rx2 * y + ry2;
        }
    }

    double p2 = ry2*(x + 0.5)*(x + 0.5) + rx2*(y - 1)*(y - 1) - rx2*ry2;

    // Región 2
    while (y >= 0) {
        drawEllipse4Points(cx, cy, x, y);
        if (p2 > 0) {
            y--;
            p2 -= two_rx2 * y + rx2;
        } else {
            y--;
            x++;
            p2 += two_ry2 * x - two_rx2 * y + rx2;
        }
    }

    glEnd();
}

// Dibuja una figura según el tipo seleccionado
void renderFigure(const Figure &fig) {
    glColor3f(fig.color.red, fig.color.green, fig.color.blue);
    switch (fig.tool) {
        case TOOL_LINE_DIRECT:
            drawLineDirect(fig.x_start, fig.y_start, fig.x_end, fig.y_end, fig.thickness);
            break;
        case TOOL_LINE_DDA:
            drawLineDDA(fig.x_start, fig.y_start, fig.x_end, fig.y_end, fig.thickness);
            break;
        case TOOL_CIRCLE_MIDPOINT:
            drawCircleMidpoint(fig.center_x, fig.center_y, fig.radius, fig.thickness);
            break;
        case TOOL_ELLIPSE_MIDPOINT:
            drawEllipseMidpoint(fig.center_x, fig.center_y, fig.radius_x, fig.radius_y, fig.thickness);
            break;
        default:
            break;
    }
}

// Redibuja todas las figuras almacenadas
void refreshScreen() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (auto &fig : figures) {
        renderFigure(fig);
    }
    glutSwapBuffers();
}

// Callbacks
void onDisplay() {
    refreshScreen();
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
        if (!isWaitingSecondClick) {
            firstClickX = x;
            firstClickY = correctedY;
            isWaitingSecondClick = true;
        } else {
            Figure fig;
            fig.tool = currentTool;
            fig.color = currentColor;
            fig.thickness = currentThickness;
            if (currentTool == TOOL_LINE_DIRECT || currentTool == TOOL_LINE_DDA) {
                fig.x_start = firstClickX;
                fig.y_start = firstClickY;
                fig.x_end = x;
                fig.y_end = correctedY;
            } else if (currentTool == TOOL_CIRCLE_MIDPOINT) {
                fig.center_x = firstClickX;
                fig.center_y = firstClickY;
                int dx = x - firstClickX;
                int dy = correctedY - firstClickY;
                fig.radius = round(sqrt(dx*dx + dy*dy));
            } else if (currentTool == TOOL_ELLIPSE_MIDPOINT) {
                fig.center_x = firstClickX;
                fig.center_y = firstClickY;
                fig.radius_x = abs(x - firstClickX);
                fig.radius_y = abs(correctedY - firstClickY);
            }
            figures.push_back(fig);
            isWaitingSecondClick = false;
            glutPostRedisplay();
        }
    }
}

// Menú de selección
void menuCallback(int option) {
    switch (option) {
        case 1: currentTool = TOOL_LINE_DIRECT; break;
        case 2: currentTool = TOOL_LINE_DDA; break;
        case 3: currentTool = TOOL_CIRCLE_MIDPOINT; break;
        case 4: currentTool = TOOL_ELLIPSE_MIDPOINT; break;
        case 10: currentColor = {0.f, 0.f, 0.f}; break;    // negro
        case 11: currentColor = {1.f, 0.f, 0.f}; break;    // rojo
        case 12: currentColor = {0.f, 0.f, 1.f}; break;    // azul
    }
    glutPostRedisplay();
}

void setupMenu() {
    int shapeMenu = glutCreateMenu(menuCallback);
    glutAddMenuEntry("Linea Directa", 1);
    glutAddMenuEntry("Linea DDA", 2);
    glutAddMenuEntry("Circulo PM", 3);
    glutAddMenuEntry("Elipse PM", 4);

    int colorMenu = glutCreateMenu(menuCallback);
    glutAddMenuEntry("Negro", 10);
    glutAddMenuEntry("Rojo", 11);
    glutAddMenuEntry("Azul", 12);

    int mainMenu = glutCreateMenu(menuCallback);
    glutAddSubMenu("Figura", shapeMenu);
    glutAddSubMenu("Color", colorMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void initializeGL() {
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glPointSize(1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Version Mejorada - Linea, Circulo y Elipse");

    initializeGL();
    setupMenu();

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutMouseFunc(onMouseClick);

    glutMainLoop();
    return 0;
}

