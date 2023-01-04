// Defined before OpenGL and GLUT includes to avoid deprecation message in OSX
#define GL_SILENCE_DEPRECATION
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyMatrix.h"
#include "cyCodeBase/cyGL.h"
#include "cyCodeBase/cyPoint.h"
#include "lodepng.h"

int width = 1280, height = 720;
int arguments = 0;
GLFWwindow* window;
cy::TriMesh objectMesh;
cy::TriMesh lightMesh;
cy::GLSLProgram progPlane, progLight, progShadowMap, progTriangulation;
bool doObjRotate = false, doObjZoom = false, doLightRotate = false, doPlaneRotate = false, doTriangulation = false;
double rotX = 5.23599, rotY = 0, distZ = 4;
float objRotX = 5.23599, objRotY= 0, objDistZ = 100.0f;
double lastX, lastY;
double lightX = -10, lightY = 30, lightZ = 25;
double shadowBias = -0.000001;
float tessLevel = 64.0f, dispScale = 10.0f;
unsigned int textureWidth = 512, textureHeight = 512;
unsigned int shadowTextureWidth = 2560, shadowTextureHeight = 1440;
std::vector<cy::Vec3f> processedVertices, processedNormals;
std::vector<cy::Vec3f> processedVerticesPlane;
std::vector<cy::Vec3f> processedVerticesTriangulation;
std::vector<cy::Vec3f> processedNormalsPlane;
std::vector<cy::Vec3f> processedNormalsTriangulation;
std::vector<cy::Vec2f> processedTexCoords;
std::vector<cy::Vec2f> processedTexCoordsPlane;
std::vector<cy::Vec2f> processedTexCoordsTriangulation;
std::vector<cy::Vec3f> processedVerticesLight;
std::vector<unsigned char> normalMapTextureData;
std::vector<unsigned char> displacementMapTextureData;
GLuint normalMapTexID, displacementMapTexID, tbo;

GLclampf Red = 0.0f, Green = 0.0f, Blue = 0.0f, Alpha = 1.0f;

void renderScene()
{
    // Clear Color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(Red, Green, Blue, Alpha);
    glDrawArrays(GL_TRIANGLES, 0, processedVertices.size());
}


bool compilePlaneShaders(int argc) {
    bool shaderSuccess;
    if (argc == 2) {
        shaderSuccess = progPlane.BuildFiles("shaderPlane.vert", "shaderPlaneNormal.frag");
    }
    else if (argc == 3) {
        shaderSuccess = progPlane.BuildFiles("shaderTriangulation.vert", "shaderPlane.frag", nullptr, "shaderTriangulation.tesscontrol", "shaderPlane.tesseval");
    }
    return shaderSuccess;
}

bool compileDepthShaders() {
    bool shaderSuccess = progShadowMap.BuildFiles("shaderTriangulation.vert", "shaderDepth.frag", nullptr, "shaderTriangulation.tesscontrol", "shaderTriangulation.tesseval");
    return shaderSuccess;
}

bool compileTriangulationShaders(int argc) {
    bool shaderSuccess;
    if (argc == 2) {
        shaderSuccess = progTriangulation.BuildFiles("shaderTriangulationNormal.vert", "shaderTriangulation.frag", "shaderTriangulation.geom");
    }
    else if (argc == 3) {
        shaderSuccess = progTriangulation.BuildFiles("shaderTriangulation.vert", "shaderTriangulation.frag", "shaderTriangulation.geom", "shaderTriangulation.tesscontrol", "shaderTriangulation.tesseval");
    }
    return shaderSuccess;
}

bool compileLightShaders() {
    bool shaderSuccess = progLight.BuildFiles("shaderLight.vert", "shaderLight.frag");
    return shaderSuccess;
}

void computeVerticesPlane() {
    processedVerticesPlane.push_back(cy::Vec3f(-50.0f, 0.0f, 50.0f));
    processedVerticesPlane.push_back(cy::Vec3f(50.0f, 0.0f, 50.0f));
    processedVerticesPlane.push_back(cy::Vec3f(-50.0f, 0.0f, -50.0f));
    processedVerticesPlane.push_back(cy::Vec3f(50.0f, 0.0f, 50.0f));
    processedVerticesPlane.push_back(cy::Vec3f(50.0f, 0.0f, -50.0f));
    processedVerticesPlane.push_back(cy::Vec3f(-50.0f, 0.0f, -50.0f));
}


void computeTexCoordsPlane() {
    processedTexCoordsPlane.push_back(cy::Vec2f(0.0f, 1.0f));
    processedTexCoordsPlane.push_back(cy::Vec2f(1.0f, 1.0f));
    processedTexCoordsPlane.push_back(cy::Vec2f(0.0f, 0.0f));
    processedTexCoordsPlane.push_back(cy::Vec2f(1.0f, 1.0f));
    processedTexCoordsPlane.push_back(cy::Vec2f(1.0f, 0.0f));
    processedTexCoordsPlane.push_back(cy::Vec2f(0.0f, 0.0f));
}

void computeNormalsPlane() {
    processedNormalsPlane.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsPlane.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsPlane.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsPlane.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsPlane.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsPlane.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
}

void computeTriangulationVertices() {
    processedVerticesTriangulation.push_back(cy::Vec3f(-50.0f, 0.0f, 50.0f));
    processedVerticesTriangulation.push_back(cy::Vec3f(50.0f, 0.0f, 50.0f));
    processedVerticesTriangulation.push_back(cy::Vec3f(50.0f, 0.0f, -50.0f));
    processedVerticesTriangulation.push_back(cy::Vec3f(-50.0f, 0.0f, -50.0f));
}

void computeTriangulationTexCoords() {
    processedTexCoordsTriangulation.push_back(cy::Vec2f(0.0f, 1.0f));
    processedTexCoordsTriangulation.push_back(cy::Vec2f(1.0f, 1.0f));
    processedTexCoordsTriangulation.push_back(cy::Vec2f(1.0f, 0.0f));
    processedTexCoordsTriangulation.push_back(cy::Vec2f(0.0f, 0.0f));
}

void computeTriangulationNormals() {
    processedNormalsTriangulation.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsTriangulation.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsTriangulation.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
    processedNormalsTriangulation.push_back(cy::Vec3f(0.0f, 1.0f, 0.0f));
}

void processNormalKeyCB(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
        case GLFW_KEY_F6:
            std::cout << "Recompiling shaders..." << std::endl;
            if (!compilePlaneShaders(arguments)) {
                std::cout << "Error Recompiling shaders" << std::endl;
            }
            if (!compileLightShaders()) {
                std::cout << "Error Recompiling shaders" << std::endl;
            }
            if (!compileTriangulationShaders(arguments)) {
                std::cout << "Error Recompiling shaders" << std::endl;
            }
            if (!compileDepthShaders()) {
                std::cout << "Error Recompiling shaders" << std::endl;
            }
            break;
        case GLFW_KEY_LEFT:
            tessLevel += 1.0f;
            std::cout << "Tess Level: " << tessLevel << std::endl;
            break;
        case GLFW_KEY_RIGHT:
            tessLevel -= 1.0f;
            break;
        case GLFW_KEY_UP:
            dispScale += 1.0f;
            break;
        case GLFW_KEY_DOWN:
            dispScale -= 1.0f;
            break;
        case GLFW_KEY_A:
            shadowBias += 0.000001f;
            break;
        case GLFW_KEY_D:
            shadowBias -= 0.000001f;
            break;
        case GLFW_KEY_LEFT_CONTROL:
        case GLFW_KEY_RIGHT_CONTROL:
            doLightRotate = true;
            break;
        case GLFW_KEY_LEFT_ALT:
        case GLFW_KEY_RIGHT_ALT:
            doPlaneRotate = true;
            break;
        case GLFW_KEY_SPACE:
            doTriangulation = !doTriangulation;
            break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_LEFT_CONTROL:
        case GLFW_KEY_RIGHT_CONTROL:
            doLightRotate = false;
            break;
        case GLFW_KEY_LEFT_ALT:
        case GLFW_KEY_RIGHT_ALT:
            doPlaneRotate = false;
            break;
        }
    }
}

static void error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}
double deg2rad (double degrees) {
    return degrees * 4.0 * atan (1.0) / 180.0;
}

void processMouseButtonCB(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        doObjZoom = true;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        doObjZoom = false;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        doObjRotate = true;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        doObjRotate = false;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    }
}

void processMousePosCB(GLFWwindow* window, double xpos, double ypos)
{
    double xDiff = lastX - xpos;
    double yDiff = lastY - ypos;
    if (doLightRotate && doObjRotate) {
        lightX += xDiff * 0.1;
        lightZ += yDiff * 0.1;
    }
    // Calculate camera zoom based on mouse movement in y direction
    if (doObjZoom && !doPlaneRotate) {
        objDistZ += yDiff * 0.1;
    }

    // Calculate plane zoom based on mouse movement in y direction
    if (doPlaneRotate && doObjZoom) {
        distZ += yDiff * 0.05;
    }

    // Calculate camera rotation based on mouse movement in x direction
    if (doObjRotate && !doLightRotate && !doPlaneRotate) {
        objRotX += yDiff * 0.005;
        objRotY += xDiff * 0.005;
    }

    // Calculate plane rotation based on mouse movement in x direction
    if (doPlaneRotate && doObjRotate) {
        rotX -= yDiff * 0.005;
        rotY -= xDiff * 0.005;
    }

    lastX  = xpos;
    lastY = ypos;
}

int main(int argc, char** argv)
{
    arguments = argc;
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW
    if (!glfwInit())
        exit(EXIT_FAILURE);
        
    // Create a windowed mode window and its OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "Shadow Mapping", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    //Initialize GLEW
    glewExperimental = GL_TRUE; 
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cout << "Error: " << glewGetErrorString(err) << std::endl;
        return 1;
    }

    // Print and test OpenGL context infos
    std::cout << glGetString(GL_VERSION) << std::endl;
    std::cout << glGetString(GL_RENDERER) << std::endl;

    // Setup GLFW callbacks
    glfwSetKeyCallback(window, processNormalKeyCB);
    glfwSetMouseButtonCallback(window, processMouseButtonCB);
    glfwSetCursorPosCallback(window, processMousePosCB);
    glfwSwapInterval(1);

    //OpenGL initializations
    // glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    // CY_GL_REGISTER_DEBUG_CALLBACK;

    //Load object normal map
    bool normalMapSuccess = lodepng::decode(normalMapTextureData, textureWidth, textureHeight, argv[1]);
    if (normalMapSuccess)
    {
        std::cout << "Error loading normal Map" << std::endl;
        return 1;
    }
    if (argc > 2)
    {
        bool displacementMapSuccess = lodepng::decode(displacementMapTextureData, textureWidth, textureHeight, argv[2]);
        if (displacementMapSuccess)
        {
            std::cout << "Error loading displacement Map" << std::endl;
            return 1;
        }
    }
    
    // Compute vertices and tex coord for plane
    computeVerticesPlane();
    computeNormalsPlane();
    computeTexCoordsPlane();
    computeTriangulationVertices();
    computeTriangulationNormals();
    computeTriangulationTexCoords();

    //Load light mesh for light source
    bool meshSuccess = lightMesh.LoadFromFileObj("sphere.obj");
    if (!meshSuccess)
    {
        std::cout << "Error loading mesh" << std::endl;
        return 1;
    }

    //Compute normals
    lightMesh.ComputeNormals();

    //Load vertices for light faces
    for (int i = 0; i < lightMesh.NF(); i++) 
    {
        cy::TriMesh::TriFace face = lightMesh.F(i);
        for (int j = 0; j < 3; j++) 
        {
            cy::Vec3f pos = lightMesh.V(face.v[j]);
            processedVerticesLight.push_back(pos);
        }
    }

    //Create vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create VBO for plane vertex data
    GLuint vboplane;
    glGenBuffers(1, &vboplane);
    glBindBuffer(GL_ARRAY_BUFFER, vboplane);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*processedVerticesPlane.size(), &processedVerticesPlane[0], GL_STATIC_DRAW);

    GLuint nboplane;
    glGenBuffers(1, &nboplane);
    glBindBuffer(GL_ARRAY_BUFFER, nboplane);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*processedNormalsPlane.size(), &processedNormalsPlane[0], GL_STATIC_DRAW);

    //Create VBO for plane texture coordinates data
    GLuint tboplane;
    glGenBuffers(1, &tboplane);
    glBindBuffer(GL_ARRAY_BUFFER, tboplane);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec2f)*processedTexCoordsPlane.size(), &processedTexCoordsPlane[0], GL_STATIC_DRAW);

    //Create VBO for light vertex data
    GLuint vbolight;
    glGenBuffers(1, &vbolight);
    glBindBuffer(GL_ARRAY_BUFFER, vbolight);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*processedVerticesLight.size(), &processedVerticesLight[0], GL_STATIC_DRAW);

    // Create VBO for triangulation vertex data
    GLuint vbotriangulation;
    glGenBuffers(1, &vbotriangulation);
    glBindBuffer(GL_ARRAY_BUFFER, vbotriangulation);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*processedVerticesTriangulation.size(), &processedVerticesTriangulation[0], GL_STATIC_DRAW);

    // Create VBO for triangulation normal data
    GLuint nbotriangulation;
    glGenBuffers(1, &nbotriangulation);
    glBindBuffer(GL_ARRAY_BUFFER, nbotriangulation);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*processedNormalsTriangulation.size(), &processedNormalsTriangulation[0], GL_STATIC_DRAW);


    // Create VBO for triangulation vertex data
    GLuint tbotriangulation;
    glGenBuffers(1, &tbotriangulation);
    glBindBuffer(GL_ARRAY_BUFFER, tbotriangulation);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec2f)*processedTexCoordsTriangulation.size(), &processedTexCoordsTriangulation[0], GL_STATIC_DRAW);

    //Setup Shader program
    std::cout << "Compiling plane shaders..." << std::endl;
    bool planeshaderSuccess = compilePlaneShaders(arguments);
    if (!planeshaderSuccess)
    {
        std::cout << "Error compiling plane shaders" << std::endl;
        return 1;
    }

    std::cout << "Compiling triangulation shaders..." << std::endl;
    bool triangulationshaderSuccess = compileTriangulationShaders(arguments);
    if (!triangulationshaderSuccess)
    {
        std::cout << "Error compiling triangulation shaders" << std::endl;
        return 1;
    }

    std::cout << "Compiling light shaders..." << std::endl;
    bool lightshaderSuccess = compileLightShaders();
    if (!lightshaderSuccess)
    {
        std::cout << "Error compiling light shaders" << std::endl;
        return 1;
    }

    std::cout << "Compiling shadow map shaders..." << std::endl;
    bool depthshaderSuccess = compileDepthShaders();
    if (!depthshaderSuccess)
    {
        std::cout << "Error compiling Shadow Map shaders" << std::endl;
        return 1;
    }

    if (argc > 1) {
        //Send normal map texture data to GPU
        glGenTextures(1, &normalMapTexID);
        glBindTexture(GL_TEXTURE_2D, normalMapTexID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &normalMapTextureData[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        //Send displacement map texture data to GPU
        if (argc > 2)
        {
            glGenTextures(1, &displacementMapTexID);
            glBindTexture(GL_TEXTURE_2D, displacementMapTexID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &displacementMapTextureData[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    }
    
    //Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Calculate camera transformations for object
        cy::Matrix4f transMatrix = cy::Matrix4f::Identity();
        cy::Matrix4f cameraPosTrans = cy::Matrix4f::RotationXYZ(objRotX, objRotY, 0.0f);
        cy::Vec4f cameraPos =  cameraPosTrans * cy::Vec4f(0, 0, objDistZ, 0);
        cy::Matrix4f viewMatrix = cy::Matrix4f::View(cy::Vec3f(cameraPos.x, cameraPos.y, cameraPos.z), cy::Vec3f(0, 0, 0), cy::Vec3f(0, 1, 0));
        cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(deg2rad(60), float(width)/float(height), 0.1f, 1000.0f);
        cy::Matrix4f m =  transMatrix;
        cy::Matrix4f mv = viewMatrix * m;
        cy::Matrix4f mvp = projMatrix * mv;

        // Render to texture binding
        cy::GLRenderDepth2D renderBuffer;
        renderBuffer.Initialize(true, shadowTextureWidth, shadowTextureHeight, GL_DEPTH_COMPONENT32F);
        if (!renderBuffer.IsReady())
        {
            std::cout << "Error initializing render buffer" << std::endl;
            return 1;
        }
        renderBuffer.BindTexture();
        renderBuffer.SetTextureWrappingMode(GL_CLAMP, GL_CLAMP);
        renderBuffer.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);

        renderBuffer.Bind();
        progShadowMap.Bind();

        cy::Matrix4f modelLight = cy::Matrix4f::Identity();
        cy::Matrix4f viewLight = cy::Matrix4f::View(cy::Vec3f(lightX, lightY, lightZ), cy::Vec3f(0, 0, 0), cy::Vec3f(0, 1, 0));
        cy::Matrix4f projLight = cy::Matrix4f::Perspective(deg2rad(90), float(width)/float(height), 0.1f, 1000.0f);;
        cy::Matrix4f mvpLight = projLight * viewLight * modelLight;
        progShadowMap["mvp"] = mvpLight;
        progShadowMap["tess_level"] = tessLevel;
        progShadowMap["displacement_scale"] = dispScale;
        progShadowMap.SetAttribBuffer("model_pos", vbotriangulation, 3);
        progShadowMap.SetAttribBuffer("model_norm", nbotriangulation, 3);
        progShadowMap.SetAttribBuffer("model_tex", tbotriangulation, 2);
        GLint normalMapTexLoc = glGetUniformLocation(progShadowMap.GetID(), "normal_map");
        GLint displacementMapTexLoc = glGetUniformLocation(progShadowMap.GetID(), "displacement_map");
        glUniform1i(normalMapTexLoc, 0);
        glUniform1i(displacementMapTexLoc, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, normalMapTexID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, displacementMapTexID);

        // Render triangulation
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(Red, Green, Blue, Alpha);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glDrawArrays(GL_PATCHES, 0, processedVerticesTriangulation.size());
        glDisable(GL_CULL_FACE);

        // Check if render buffer is ready
        if (!renderBuffer.IsComplete())
        {
            std::cout << "Error completing render buffer" << std::endl;
            return 1;
        }
        renderBuffer.Unbind();

        // Set Program and Program Attributes for plane
        progPlane.Bind();
        progPlane["mvp"] = mvp;
        progPlane["m"] = m;
        progPlane["mv"] = mv;
        progPlane["camera_pos"] = cameraPos;
        progPlane["light_pos"] = viewMatrix * cy::Vec4f(lightX, lightY, lightZ, 0);
        if (argc > 2) {
            progPlane["mvp_light"] = cy::Matrix4f::Translation(cy::Vec3f(0.5f, 0.5f, 0.5f - shadowBias)) * cy::Matrix4f::Scale(0.5f, 0.5f, 0.5f) * mvpLight;
            progPlane["tess_level"] = tessLevel;
            progPlane["displacement_scale"] = dispScale;
            progPlane.SetAttribBuffer("model_pos", vbotriangulation, 3);
            progPlane.SetAttribBuffer("model_norm", nbotriangulation, 3);
            progPlane.SetAttribBuffer("model_tex", tbotriangulation, 2);

            GLint normalMapTexLoc = glGetUniformLocation(progPlane.GetID(), "normal_map");
            GLint displacementMapTexLoc = glGetUniformLocation(progPlane.GetID(), "displacement_map");
            GLint shadowMapTexLoc = glGetUniformLocation(progPlane.GetID(), "model_shadow");
            glUniform1i(normalMapTexLoc, 0);
            glUniform1i(displacementMapTexLoc, 1);
            glUniform1i(shadowMapTexLoc, 2);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, normalMapTexID);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, displacementMapTexID);
            renderBuffer.BindTexture(2);

            // Render plane
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glClearColor(Red, Green, Blue, Alpha);
            glPatchParameteri(GL_PATCH_VERTICES, 4);
            glDrawArrays(GL_PATCHES, 0, processedVerticesTriangulation.size());

        } else {
            progPlane.SetAttribBuffer("pos", vboplane, 3);
            progPlane.SetAttribBuffer("norm", nboplane, 3);
            progPlane.SetAttribBuffer("tex", tboplane, 2);
            GLint normalMapTexLoc = glGetUniformLocation(progPlane.GetID(), "normal_map");
            glUniform1i(normalMapTexLoc, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, normalMapTexID);
        
            // Render plane
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glClearColor(Red, Green, Blue, Alpha);
            glDrawArrays(GL_TRIANGLES, 0, processedVerticesPlane.size());
        }

        // Set Program and program attributes for triangulation
        if (doTriangulation) {            
            progTriangulation.Bind();
            m = cy::Matrix4f::Translation(cy::Vec3f(0, 0.1f, 0));
            mv = viewMatrix * m;
            mvp = projMatrix * mv;
            progTriangulation["mvp"] = mvp;
            if (argc > 2) {
                progTriangulation["tess_level"] = tessLevel;
                progTriangulation["displacement_scale"] = dispScale;
                progTriangulation.SetAttribBuffer("model_pos", vbotriangulation, 3);
                progTriangulation.SetAttribBuffer("model_norm", nbotriangulation, 3);
                progTriangulation.SetAttribBuffer("model_tex", tbotriangulation, 2);
                GLint displacementMapTexLoc = glGetUniformLocation(progTriangulation.GetID(), "displacement_map");
                glUniform1i(displacementMapTexLoc, 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, displacementMapTexID);

                // Render triangulation
                glPatchParameteri(GL_PATCH_VERTICES, 4);
                glDrawArrays(GL_PATCHES, 0, processedVerticesTriangulation.size());
            } else {
                // Render triangulation
                progTriangulation.SetAttribBuffer("pos", vboplane, 3);
                glDrawArrays(GL_TRIANGLES, 0, processedVerticesPlane.size());
            }
        }

        // Draw light source
        progLight.Bind();
        modelLight = cy::Matrix4f::Translation(cy::Vec3f(lightX, lightY, lightZ)) * cy::Matrix4f::Scale(0.2f);
        viewLight = viewMatrix;
        projLight = projMatrix;
        mvpLight = projLight * viewLight * modelLight;
        progLight["mvp"] = mvpLight;
        progLight.SetAttribBuffer("pos", vbolight, 3);

        glDrawArrays(GL_TRIANGLES, 0, processedVerticesLight.size());

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}