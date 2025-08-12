#include "Form.hpp"
#include <cmath>

static void drawUnitCube();
static void drawUnitSphere(int subdivisions = 16);

void Form::draw() const {
    switch (shape) {
        case ShapeType::Cube:
            glPushMatrix();
            glScalef(dimensions.x, dimensions.y, dimensions.z);
            drawUnitCube();
            glPopMatrix();
            break;
        case ShapeType::Sphere:
            glPushMatrix();
            glScalef(dimensions.x, dimensions.y, dimensions.z);
            drawUnitSphere();
            glPopMatrix();
            break;
        case ShapeType::Custom:
        default:
            // Custom shapes not yet implemented
            break;
    }
}

// -------- helper implementations -----------------------------
static void drawUnitCube() {
    glBegin(GL_QUADS);
    // Front
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    // Back
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    // Left
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    // Right
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f,  0.5f, -0.5f);
    glVertex3f(0.5f,  0.5f,  0.5f);
    glVertex3f(0.5f, -0.5f,  0.5f);
    // Top
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f, -0.5f);
    // Bottom
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glEnd();
}

static void drawUnitSphere(int subdivisions) {
    for (int i = 0; i <= subdivisions; ++i) {
        float lat0 = M_PI * (-0.5f + (float) (i - 1) / subdivisions);
        float z0  = sin(lat0);
        float zr0 = cos(lat0);

        float lat1 = M_PI * (-0.5f + (float) i / subdivisions);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= subdivisions; ++j) {
            float lng = 2 * M_PI * (float) (j - 1) / subdivisions;
            float x = cos(lng);
            float y = sin(lng);

            glVertex3f(x * zr0 * 0.5f, y * zr0 * 0.5f, z0 * 0.5f);
            glVertex3f(x * zr1 * 0.5f, y * zr1 * 0.5f, z1 * 0.5f);
        }
        glEnd();
    }
}
