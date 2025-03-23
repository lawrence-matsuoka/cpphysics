#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Constants
const float G = 6.67430e-11f;  // Gravitational constant
const float WINDOW_WIDTH = 800;
const float WINDOW_HEIGHT = 600;
const float MOVEMENT_SPEED = 1.0f;
const float MOUSE_SENSITIVITY = 0.05f;

// Shader sources (basic vertex and fragment shaders)
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
})";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main()
{
    FragColor = vec4(color, 1.0f);
})";

// Structure to represent a 3D vector (position or velocity)
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // Overload + operator to add two Vec3s
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    // Overload - operator to subtract two Vec3s
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    // Overload * operator to scale Vec3 by a scalar
    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    // Normalize the vector
    Vec3 normalize() const {
        float len = length();
        if (len > 0) return *this * (1.0f / len);
        return *this;
    }

    // Compute the length of the vector
    float length() const {
        return sqrt(x * x + y * y + z * z);
    }
};

// Structure to represent a body (mass, position, velocity, color)
struct Body {
    float mass;
    Vec3 position;
    Vec3 velocity;
    float radius;
    float r, g, b;  // Color

    Body(float m, Vec3 pos, Vec3 vel, float rad, float red, float green, float blue)
        : mass(m), position(pos), velocity(vel), radius(rad), r(red), g(green), b(blue) {}
};

// Function to compute the gravitational force between two bodies
Vec3 computeGravitationalForce(const Body& body1, const Body& body2) {
    Vec3 diff = body2.position - body1.position;
    float dist = diff.length();
    float forceMagnitude = (G * body1.mass * body2.mass) / (dist * dist);
    Vec3 force = diff * (forceMagnitude / dist);  // Force vector points towards body2
    return force;
}

// Function to update positions and velocities of bodies using basic physics
void updatePhysics(std::vector<Body>& bodies, float dt) {
    for (size_t i = 0; i < bodies.size(); ++i) {
        Vec3 netForce(0.0f, 0.0f, 0.0f);
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i != j) {
                netForce = netForce + computeGravitationalForce(bodies[i], bodies[j]);
            }
        }

        // Update velocity based on net force (F = ma => a = F/m)
        Vec3 acceleration = netForce * (1.0f / bodies[i].mass);
        bodies[i].velocity = bodies[i].velocity + acceleration * dt;

        // Update position based on velocity (v = ds/dt => s = s + v*dt)
        bodies[i].position = bodies[i].position + bodies[i].velocity * dt;
    }
}

// Function to create a simple sphere (to represent bodies)
GLuint createSphere(float radius, int slices, int stacks) {
    GLuint VAO, VBO;
    std::vector<GLfloat> vertices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = M_PI * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            float theta = 2 * M_PI * j / slices;
            float x = radius * sin(phi) * cos(theta);
            float y = radius * sin(phi) * sin(theta);
            float z = radius * cos(phi);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return VAO;
}

// Function to create and compile a shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

// Function to create shader program
GLuint createShader(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Function to handle keyboard input
void processInput(GLFWwindow* window, glm::vec3& cameraPos, float deltaTime) {
    float cameraSpeed = MOVEMENT_SPEED * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos.z -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos.z += cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos.x -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos.x += cameraSpeed;
}

// Main program
int main() {
    srand(static_cast<unsigned int>(time(0)));

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW Initialization Failed!" << std::endl;
        return -1;
    }

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "3-Body Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window Creation Failed!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW Initialization Failed!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);  // Enable depth testing

    // Set up shader program
    GLuint shaderProgram = createShader(vertexShaderSource, fragmentShaderSource);

    // Create a sphere for each body
    GLuint sphereVAO = createSphere(0.05f, 20, 20);

    // Create three bodies with random positions and velocities
    std::vector<Body> bodies;
    bodies.push_back(Body(1e10f, Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f), 0.1f, 1.0f, 0.0f, 0.0f));
    bodies.push_back(Body(1e10f, Vec3(2.0f, 0.0f, 0.0f), Vec3(0.0f, 0.5f, 0.0f), 0.1f, 0.0f, 1.0f, 0.0f));
    bodies.push_back(Body(1e10f, Vec3(0.0f, 2.0f, 0.0f), Vec3(0.0f, -0.5f, 0.0f), 0.1f, 0.0f, 0.0f, 1.0f));

    // Camera position
    glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        float deltaTime = glfwGetTime();
        glfwSetTime(0.0f); // Reset time for next frame

        // Handle input
        processInput(window, cameraPos, deltaTime);

        // Update physics
        updatePhysics(bodies, deltaTime);

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Set uniforms for view and projection
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw each body (sphere)
        for (size_t i = 0; i < bodies.size(); ++i) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(bodies[i].position.x, bodies[i].position.y, bodies[i].position.z));
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            GLuint colorLoc = glGetUniformLocation(shaderProgram, "color");
            glUniform3f(colorLoc, bodies[i].r, bodies[i].g, bodies[i].b);

            glBindVertexArray(sphereVAO);
            glDrawArrays(GL_POINTS, 0, 400);  // Draw as points for simplicity
        }

        // Swap buffers and poll for events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up and exit
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
