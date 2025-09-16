#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <stack>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Tipos de dibujo disponibles
enum Tool {
    TOOL_LINE_DIRECT,
    TOOL_LINE_DDA,
    TOOL_CIRCLE_MIDPOINT,
    TOOL_ELLIPSE_MIDPOINT,
    TOOL_NONE
};

// Color RGB
struct ColorRGB {
    float r, g, b;
};

// Estructura para figuras geométricas
struct Figure {
    Tool toolType;
    // Líneas: puntos inicio-fin
    int xStart, yStart, xEnd, yEnd;
    // Círculo: centro y radio
    int centerX, centerY, radius;
    // Elipse: radios horizontal y vertical
    int radiusX, radiusY;
    ColorRGB color;
    int thickness;
};

vector<Figure> figures;
stack<vector<Figure>> undoStack;
stack<vector<Figure>> redoStack;

// Estado actual
Tool currentTool = TOOL_LINE_DIRECT;
ColorRGB currentColor = {0.f, 0.f, 0.f};
int currentThickness = 1;
bool showGrid = true;
bool showAxes = true;
bool waitingSecondClick = false;
int firstX = 0;
int firstY = 0;
int viewportWidth = WINDOW_WIDTH;
int viewportHeight = WINDOW_HEIGHT;

inline int roundToInt(float v) {
    return (int) floor(v + 0.5f);
}

// Gestión Undo/Redo
void pushUndo() {
    undoStack.push(figures);
    while (!redoStack.empty()) redoStack.pop();
}

void undo() {
    if (!undoStack.empty()) {
        redoStack.push(figures);
        figures = undoStack.top();
        undoStack.pop();
        glutPostRedisplay();
    }
}

void redo() {
    if (!redoStack.empty()) {
        undoStack.push(figures);
        figures = redoStack.top();
        redoStack.pop();
        glutPostRedisplay();
    }
}

// Dibujar punto en (x,y)
void drawPixel(int x, int y) {
    glVertex2i(x, y);
}

// Algoritmos línea directa y DDA
void drawLineDirect(int x0, int y0, int x1, int y1, int thickness) {
    int dx = x1 - x0, dy = y1 - y0;
    glPointSize(thickness);
    glBegin(GL_POINTS);
    if (dx == 0) {
        int stepY = (y1 > y0) ? 1 : -1;
        for (int y = y0; y != y1 + stepY; y += stepY) drawPixel(x0, y);
    } else if (dy == 0) {
        int stepX = (x1 > x0) ? 1 : -1;
        for (int x = x0; x != x1 + stepX; x += stepX) drawPixel(x, y0);
    } else {
        float m = (float) dy / dx;
        if (fabs(m) <= 1) {
            int stepX = (x1 > x0) ? 1 : -1;
            for (int x = x0; x != x1 + stepX; x += stepX) drawPixel(x, roundToInt(m*(x - x0) + y0));
        } else {
            int stepY = (y1 > y0) ? 1 : -1;
            float invM = (float) dx / dy;
            for (int y = y0; y != y1 + stepY; y += stepY) drawPixel(roundToInt(invM*(y - y0) + x0), y);
        }
    }
    glEnd();
}

void drawLineDDA(int x0, int y0, int x1, int y1, int thickness) {
    int dx = x1 - x0, dy = y1 - y0;
    int steps = max(abs(dx), abs(dy));
    float x = x0, y = y0;
    float incX = dx / (float) steps;
    float incY = dy / (float) steps;
    glPointSize(thickness);
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++) {
        drawPixel(roundToInt(x), roundToInt(y));
        x += incX;
        y += incY;
    }
    glEnd();
}

// Círculo y Elipse punto medio
void drawCirclePoints(int cx, int cy, int x, int y) {
    drawPixel(cx + x, cy + y);
    drawPixel(cx - x, cy + y);
    drawPixel(cx + x, cy - y);
    drawPixel(cx - x, cy - y);
    drawPixel(cx + y, cy + x);
    drawPixel(cx - y, cy + x);
    drawPixel(cx + y, cy - x);
    drawPixel(cx - y, cy - x);
}

void drawCircleMidpoint(int cx, int cy, int r, int thickness) {
    int x = 0, y = r;
    int p = 1 - r;
    glPointSize(thickness);
    glBegin(GL_POINTS);
    drawCirclePoints(cx, cy, x, y);
    while (x < y) {
        x++;
        if (p < 0) p += 2*x + 1;
        else {
            y--;
            p += 2*(x - y) + 1;
        }
        drawCirclePoints(cx, cy, x, y);
    }
    glEnd();
}

void drawEllipsePoints(int cx, int cy, int x, int y) {
    drawPixel(cx + x, cy + y);
    drawPixel(cx - x, cy + y);
    drawPixel(cx + x, cy - y);
    drawPixel(cx - x, cy - y);
}

void drawEllipseMidpoint(int cx, int cy, int rx, int ry, int thickness) {
    int x = 0, y = ry;
    long rx2 = rx*rx, ry2 = ry*ry;
    long two_rx2 = 2*rx2, two_ry2 = 2*ry2;
    double p1 = ry2 - rx2 * ry + 0.25*rx2;
    glPointSize(thickness);
    glBegin(GL_POINTS);
    while (two_ry2*x <= two_rx2*y) {
        drawEllipsePoints(cx, cy, x, y);
        if (p1 < 0) {
            x++;
            p1 += two_ry2*x + ry2;
        } else {
            x++;
            y--;
            p1 += two_ry2*x - two_rx2*y + ry2;
        }
    }
    double p2 = ry2*(x+0.5)*(x+0.5) + rx2*(y-1)*(y-1) - rx2*ry2;
    while (y >= 0) {
        drawEllipsePoints(cx, cy, x, y);
        if (p2 > 0) {
            y--;
            p2 -= two_rx2*y + rx2;
        } else {
            y--;
            x++;
            p2 += two_ry2*x - two_rx2*y + rx2;
        }
    }
    glEnd();
}

void drawFigure(const Figure &f) {
    glColor3f(f.color.r, f.color.g, f.color.b);
    switch (f.toolType) {
        case TOOL_LINE_DIRECT:
            drawLineDirect(f.xStart, f.yStart, f.xEnd, f.yEnd, f.thickness);
            break;
        case TOOL_LINE_DDA:
            drawLineDDA(f.xStart, f.yStart, f.xEnd, f.yEnd, f.thickness);
            break;
        case TOOL_CIRCLE_MIDPOINT:
            drawCircleMidpoint(f.centerX, f.centerY, f.radius, f.thickness);
            break;
        case TOOL_ELLIPSE_MIDPOINT:
            drawEllipseMidpoint(f.centerX, f.centerY, f.radiusX, f.radiusY, f.thickness);
            break;
        default:
            break;
    }
}

void redrawAll() {
    glClear(GL_COLOR_BUFFER_BIT);
    // Dibujar cuadrícula
    if (showGrid) {
        glColor3f(0.85f, 0.85f, 0.85f);
        glBegin(GL_LINES);
        for (int x = 0; x <= viewportWidth; x += 20) {
            glVertex2i(x, 0);
            glVertex2i(x, viewportHeight);
        }
        for (int y = 0; y <= viewportHeight; y += 20) {
            glVertex2i(0, y);
            glVertex2i(viewportWidth, y);
        }
        glEnd();
    }
    // Dibujar ejes
    if (showAxes) {
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_LINES);
        glVertex2i(0, viewportHeight / 2);
        glVertex2i(viewportWidth, viewportHeight / 2);
        glVertex2i(viewportWidth / 2, 0);
        glVertex2i(viewportWidth / 2, viewportHeight);
        glEnd();
    }
    // Dibujar todas las figuras
    for (auto &f : figures) {
        drawFigure(f);
    }
    glutSwapBuffers();
}

void exportToPPM(const string &filename) {
    int w = viewportWidth, h = viewportHeight;
    vector<unsigned char> pixels(3 * w * h);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    ofstream ofs(filename, ios::binary);
    ofs << "P6\n" << w << " " << h << "\n255\n";
    for (int y = h - 1; y >= 0; y--) {
        ofs.write((char*)&pixels[y * w * 3], 3 * w);
    }
    ofs.close();
    cout << "Imagen exportada a " << filename << endl;
}

// Callbacks
void display() {
    redrawAll();
}

void reshape(int w, int h) {
    viewportWidth = w;
    viewportHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void mouse(int button, int state, int x, int y) {
    int ox = x;
    int oy = viewportHeight - y;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (!waitingSecondClick) {
            firstX = ox;
            firstY = oy;
            waitingSecondClick = true;
        } else {
            pushUndo();
            Figure f;
            f.color = currentColor;
            f.thickness = currentThickness;
            switch (currentTool) {
                case TOOL_LINE_DIRECT:
                case TOOL_LINE_DDA:
                    f.toolType = currentTool;
                    f.xStart = firstX;
                    f.yStart = firstY;
                    f.xEnd = ox;
                    f.yEnd = oy;
                    break;
                case TOOL_CIRCLE_MIDPOINT:
                    f.toolType = TOOL_CIRCLE_MIDPOINT;
                    f.centerX = firstX;
                    f.centerY = firstY;
                    f.radius = round(sqrt(pow(ox - firstX, 2) + pow(oy - firstY, 2)));
                    break;
                case TOOL_ELLIPSE_MIDPOINT:
                    f.toolType = TOOL_ELLIPSE_MIDPOINT;
                    f.centerX = firstX;
                    f.centerY = firstY;
                    f.radiusX = abs(ox - firstX);
                    f.radiusY = abs(oy - firstY);
                    break;
                default:
                    break;
            }
            figures.push_back(f);
            waitingSecondClick = false;
            glutPostRedisplay();
        }
    }
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 'g': case 'G': showGrid = !showGrid; break;
        case 'e': case 'E': showAxes = !showAxes; break;
        case 'c': case 'C': pushUndo(); figures.clear(); break;
        case 'z': case 'Z': undo(); break;
        case 'y': case 'Y': redo(); break;
        case 'p': case 'P': case 's': case 'S': exportToPPM("canvas.ppm"); break;
        case 27: exit(0); break;  // ESC para salir
        default: break;
    }
    glutPostRedisplay();
}

void menuHandler(int option) {
    switch (option) {
        case 1: currentTool = TOOL_LINE_DIRECT; break;
        case 2: currentTool = TOOL_LINE_DDA; break;
        case 3: currentTool = TOOL_CIRCLE_MIDPOINT; break;
        case 4: currentTool = TOOL_ELLIPSE_MIDPOINT; break;
        case 10: currentColor = {0.f, 0.f, 0.f}; break;    // Negro
        case 11: currentColor = {1.f, 0.f, 0.f}; break;    // Rojo
        case 12: currentColor = {0.f, 1.f, 0.f}; break;    // Verde
        case 13: currentColor = {0.f, 0.f, 1.f}; break;    // Azul
        case 20: currentThickness = 1; break;
        case 21: currentThickness = 2; break;
        case 22: currentThickness = 3; break;
        case 23: currentThickness = 5; break;
        case 30: showGrid = !showGrid; break;
        case 31: showAxes = !showAxes; break;
        case 40: pushUndo(); figures.clear(); break;
        case 41: undo(); break;
        case 42: redo(); break;
        case 43: exportToPPM("canvas.ppm"); break;
    }
    glutPostRedisplay();
}

void createMenus() {
    int drawMenu = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Linea Directa", 1);
    glutAddMenuEntry("Linea DDA", 2);
    glutAddMenuEntry("Circulo PM", 3);
    glutAddMenuEntry("Elipse PM", 4);

    int colorMenu = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Negro", 10);
    glutAddMenuEntry("Rojo", 11);
    glutAddMenuEntry("Verde", 12);
    glutAddMenuEntry("Azul", 13);

    int thicknessMenu = glutCreateMenu(menuHandler);
    glutAddMenuEntry("1px", 20);
    glutAddMenuEntry("2px", 21);
    glutAddMenuEntry("3px", 22);
    glutAddMenuEntry("5px", 23);

    int viewMenu = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Mostrar/Ocultar Grid", 30);
    glutAddMenuEntry("Mostrar/Ocultar Ejes", 31);

    int toolsMenu = glutCreateMenu(menuHandler);
    glutAddMenuEntry("Limpiar", 40);
    glutAddMenuEntry("Deshacer", 41);
    glutAddMenuEntry("Rehacer", 42);
    glutAddMenuEntry("Exportar PPM", 43);

    int mainMenu = glutCreateMenu(menuHandler);
    glutAddSubMenu("Dibujo", drawMenu);
    glutAddSubMenu("Color", colorMenu);
    glutAddSubMenu("Grosor", thicknessMenu);
    glutAddSubMenu("Vista", viewMenu);
    glutAddSubMenu("Herramientas", toolsMenu);

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
    glutCreateWindow("Mini CAD Raster");

    initializeGL();
    createMenus();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}

