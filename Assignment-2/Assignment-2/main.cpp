// Std. Includes
#include <string>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GL includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Text.h"
#include "Texture.h"

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

// Other Libs
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "SOIL2/SOIL2.h"

// Text Rendering
#include <ft2build.h>
#include FT_FREETYPE_H


#define IMGUI_GLSL_VERSION      "#version 330 core"

// Properties
const GLuint WIDTH = 1600, HEIGHT = 1000;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Function prototypes
void KeyCallback( GLFWwindow *window, int key, int scancode, int action, int mode );
void MouseCallback( GLFWwindow *window, double xPos, double yPos );
void DoMovement( );

void ImGuiInit();
void ImGuiWindowing();
GLfloat CamX = 0.0f;
GLfloat CamY = 0.0f;
GLfloat CamZ = 3.0f;

Camera camera( glm::vec3( CamX, CamY, CamZ ) );
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLfloat movementSpeed = 0.075f;
glm::vec3 BallMovement = glm::vec3(0.0f, 0.0f, 0.0f);

float upperArmAngle;
float lowerArmAngle;

int effectors = 3;

int armLengths = 2;
int armLength = 2;
glm::vec3 links[3];
float angles[3];

glm::vec3 startingPos(4.0f, 0.0f, 0.0f);

void setUpLinks() {
    links[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    links[1] = glm::vec3(2.0f, 0.0f, 0.0f);
    links[2] = glm::vec3(4.0f, 0.0f, 0.0f);
}

void blinnPhongLighting(Shader shader){
    GLint lightDirLoc = glGetUniformLocation( shader.Program, "light.direction" );
    GLint viewPosLoc  = glGetUniformLocation( shader.Program, "viewPos" );
    
    glUniform3f( lightDirLoc, -5.0f, 0.5f, -1.5f);
    glUniform3f( viewPosLoc,  camera.GetPosition( ).x, camera.GetPosition( ).y, camera.GetPosition( ).z );
    
    // LIGHT PROPERTIES
    glUniform3f( glGetUniformLocation( shader.Program, "light.ambient" ), 0.5f, 0.5f, 0.5f );
    glUniform3f( glGetUniformLocation( shader.Program, "light.diffuse" ), 0.5f, 0.5f, 0.5f );
    glUniform3f( glGetUniformLocation( shader.Program, "light.specular" ), 1.0f, 1.0f, 1.0f );

    glUniform1f( glGetUniformLocation( shader.Program, "shininess" ), 32.0f );
}


// ANALYTICAL
void calculateAngles(float x, float y) {
    float l1 = 0, l2 = 0;
    float ex = 0, ey = 0;
    float cos2 = 0;
    ex = x;
    ey = y;
    
    float d = sqrt((x*x) + (y*y));
    
    l1 = armLength;
    l2 = armLength;
    
    float cosThetaT = y / x;
    float ThetaT = (float)glm::atan(cosThetaT);

    float cosTheta1MinusThetaT = (l1*l1 + ex*ex + ey*ey - l2*l2) / (2 * l1 * (d));
    float Theta1 = (float)glm::acos(cosTheta1MinusThetaT) + ThetaT;

    float Theta2 = (float)glm::acos(((x*x + y*y) - (l1*l1 + l2*l2)) / (2 * l1 * l2));
    
    cos2 = ((ex * ex) + (ey * ey) - (l1 * l1) - (l2 * l2)) / (2 * l1 * l2);

    if (cos2 >= -1.0 && cos2 <= 1.0) {
        lowerArmAngle = -glm::degrees(Theta2);
        upperArmAngle = glm::degrees(Theta1);
    }
    
}

// CCD
double VectorSquaredDistance(glm::vec3 *v1, glm::vec3 *v2)
{
    return(((v1->x - v2->x) * (v1->x - v2->x)) +
        ((v1->y - v2->y) * (v1->y - v2->y)) +
        ((v1->z - v2->z) * (v1->z - v2->z)));
}

double VectorSquaredLength(glm::vec3 *v)
{
    return((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
}

double VectorLength(glm::vec3 *v)
{
    return(sqrt(VectorSquaredLength(v)));
}

void NormalizeVector(glm::vec3 *v)
{
    float len = (float)VectorLength(v);
    if (len != 0.0)
    {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}

double DotProduct(glm::vec3 *v1, glm::vec3 *v2)
{
    return ((v1->x * v2->x) + (v1->y * v2->y) + (v1->z + v2->z));
}

void CrossProduct(glm::vec3 *v1, glm::vec3 *v2, glm::vec3 *result)
{
    result->x = (v1->y * v2->z) - (v1->z * v2->y);
    result->y = (v1->z * v2->x) - (v1->x * v2->z);
    result->z = (v1->x * v2->y) - (v1->y * v2->x);
}

void ComputeCCD(int x, int y) {
    glm::vec3 rootPos, curEnd, desiredEnd, targetVector, curVector, crossResult;
    double cosAngle, turnAngle, turnDeg;
    int link, tries;
    bool found = false;

    link = effectors - 1;
    tries = 0;

    while (tries < 100 && link >= 0) {
        rootPos.x = links[link].x;
        rootPos.y = links[link].y;
        rootPos.z = links[link].z;

        curEnd.x = links[effectors].x;
        curEnd.y = links[effectors].y;
        curEnd.z = 0.0f;

        desiredEnd.x = float(x);
        desiredEnd.y = float(y);
        desiredEnd.z = 0.0f;

        if (VectorSquaredDistance(&curEnd, &desiredEnd) > 1.0f) {
            curVector.x = curEnd.x - rootPos.x;
            curVector.y = curEnd.y - rootPos.y;
            curVector.z = curEnd.z - rootPos.z;

            targetVector.x = x - rootPos.x;
            targetVector.y = y - rootPos.y;
            targetVector.z = 0.0f;

            NormalizeVector(&curVector);
            NormalizeVector(&targetVector);

            cosAngle = DotProduct(&targetVector, &curVector);

            if (cosAngle < 0.99999) {
                CrossProduct(&targetVector, &curVector, &crossResult);
                if (crossResult.z > 0.0f) {
                    turnAngle = glm::acos((float)cosAngle);
                    turnDeg = glm::degrees(turnAngle);
                    angles[link] -= (float)turnDeg;
                }
                else if (crossResult.z < 0.0f) {
                    turnAngle = glm::acos((float)cosAngle);
                    turnDeg = glm::degrees(turnAngle);
                    angles[link] += (float)turnDeg;
                }
            }
        }

        tries++;
        link--;
    }

}

static GLFWwindow *window = nullptr;
int main( )
{
    setUpLinks();
    glfwInit( );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );
    window = glfwCreateWindow( WIDTH, HEIGHT, "Quaternion Rotations", nullptr, nullptr );
    
    if ( nullptr == window )
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate( );
        
        return EXIT_FAILURE;
    }
    
    glfwMakeContextCurrent( window );
    glfwGetFramebufferSize( window, &SCREEN_WIDTH, &SCREEN_HEIGHT );
    glfwSetKeyCallback( window, KeyCallback );
    glfwSetCursorPosCallback( window, MouseCallback );
    glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
    glewExperimental = GL_TRUE;
    if ( GLEW_OK != glewInit( ) )
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Define the viewport dimensions
    glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
    
    // OpenGL options
    glEnable( GL_DEPTH_TEST );
    
    // LIGHTING
    Shader blinnPhongShader( "res/shaders/blinnPhongVS.vs", "res/shaders/blinnPhongFS.frag" );
    
    // MODELS
    Model Ball( "res/models/Ball.obj" );
    Model Body( "res/models/Body.obj" );
    Model Limb( "res/models/Limb.obj" );
   

    // SKYBOX
    Shader skyboxShader( "res/shaders/skybox.vs", "res/shaders/skybox.frag" );

    GLfloat skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };
    
    // Setup skybox VAO
    GLuint skyboxVAO, skyboxVBO;
    glGenVertexArrays( 1, &skyboxVAO );
    glGenBuffers( 1, &skyboxVBO );
    glBindVertexArray( skyboxVAO );
    glBindBuffer( GL_ARRAY_BUFFER, skyboxVBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( skyboxVertices ), &skyboxVertices, GL_STATIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( GLfloat ), ( GLvoid * ) 0 );
    glBindVertexArray(0);
    
    // Cubemap (Skybox)
    vector<const GLchar*> faces;
    faces.push_back( "res/images/skybox/Tower/px.png" );
    faces.push_back( "res/images/skybox/Tower/nx.png" );
    faces.push_back( "res/images/skybox/Tower/py.png" );
    faces.push_back( "res/images/skybox/Tower/ny.png" );
    faces.push_back( "res/images/skybox/Tower/pz.png" );
    faces.push_back( "res/images/skybox/Tower/nz.png" );
    GLuint cubemapTexture = TextureLoading::LoadCubemap( faces );

    
    glm::mat4 projection = glm::perspective( camera.GetZoom( ), ( float )SCREEN_WIDTH/( float )SCREEN_HEIGHT, 0.1f, 1000.0f );

    
    ImGuiInit();
    
    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // SETUP TEXT SHADER
    // ----------------------------
    Shader textShader("res/shaders/text.vs", "res/shaders/text.frag");
    textShader.Use();
    glm::mat4 projection_text = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(textShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection_text));
    
    Text text;
    
    // GAME LOOP
    while( !glfwWindowShouldClose( window ) )
    {
        GLfloat currentFrame = glfwGetTime( );
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        glfwPollEvents( );
        DoMovement( );
        
        // COLOR BUFFER CLEAR
        glClearColor(0.5f, 0.5f, 0.5f, 0.1f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        
        glm::mat4 view = camera.GetViewMatrix( );
        
        calculateAngles(startingPos.x, startingPos.y);
//        ComputeCCD(startingPos.x, startingPos.y);
        blinnPhongShader.Use( );
        
        
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "projection" ), 1, GL_FALSE, glm::value_ptr( projection ) );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "view" ), 1, GL_FALSE, glm::value_ptr( view ) );
        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );
        
        glm::mat4 upperArmModel = glm::mat4(1.0f);
        upperArmModel = glm::rotate(upperArmModel, glm::radians(upperArmAngle), glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel ) );
        Limb.Draw( blinnPhongShader );

        // Lower Arm
        glm::mat4 lowerArmModel = glm::mat4(1.0f);
        lowerArmModel = glm::translate(lowerArmModel, glm::vec3(2.0f, 0.0f, 0.0f));
        lowerArmModel = glm::rotate(lowerArmModel, glm::radians(lowerArmAngle), glm::vec3(0.0f, 0.0f, 1.0f));
        lowerArmModel = upperArmModel * lowerArmModel;

        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel ) );
        Limb.Draw( blinnPhongShader );

        /*
        glm::mat4 upperArmModel = glm::mat4(1.0f);
        upperArmModel = glm::rotate(upperArmModel, glm::radians(angles[0]), glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel ) );
        Limb.Draw( blinnPhongShader );

        // Lower Arm
        glm::mat4 lowerArmModel = glm::mat4(1.0f);
        lowerArmModel = glm::translate(lowerArmModel, glm::vec3(2.0f, 0.0f, 0.0f));
        lowerArmModel = glm::rotate(lowerArmModel, glm::radians(angles[1]), glm::vec3(0.0f, 0.0f, 1.0f));
        lowerArmModel = upperArmModel * lowerArmModel;

        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel ) );
        Limb.Draw( blinnPhongShader );

        // Link 3
        glm::mat4 link3Model = glm::mat4(1.0f);
        link3Model = glm::translate(link3Model, glm::vec3(2.0f, 0.0f, 0.0f));
        link3Model = glm::rotate(link3Model, glm::radians(angles[2]), glm::vec3(0.0f, 0.0f, 1.0f));
        link3Model = lowerArmModel * link3Model;
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( link3Model ) );
        Limb.Draw( blinnPhongShader );
        */
         
//        glm::mat4 BodyModel = glm::mat4(1.0f);
////        BodyModel = glm::scale(BodyModel, glm::vec3(0.1f));
//        BodyModel = glm::translate(BodyModel, glm::vec3(-1.0f, 0.0f, 0.0f));
//        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( BodyModel ) );
//        Body.Draw( blinnPhongShader );
//        blinnPhongLighting( blinnPhongShader );
//        
        glm::mat4 BallModel = glm::mat4(1.0f);
//        BallModel = glm::scale(BallModel, glm::vec3(0.1f));
        BallModel = glm::translate(BallModel, startingPos);
        blinnPhongShader.Use( );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "projection" ), 1, GL_FALSE, glm::value_ptr( projection ) );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "view" ), 1, GL_FALSE, glm::value_ptr( view ) );
        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( BallModel ) );
        Ball.Draw( blinnPhongShader );
        blinnPhongLighting( blinnPhongShader );
        
        text.RenderText(textShader, "", 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
        
        // SKYBOX
        glDepthFunc( GL_LEQUAL );
        skyboxShader.Use( );
        view = glm::mat4( glm::mat3( camera.GetViewMatrix( ) ) );
        glUniformMatrix4fv( glGetUniformLocation( skyboxShader.Program, "view" ), 1, GL_FALSE, glm::value_ptr( view ) );
        glUniformMatrix4fv( glGetUniformLocation( skyboxShader.Program, "projection" ), 1, GL_FALSE, glm::value_ptr( projection ) );
        glBindVertexArray( skyboxVAO );
        glBindTexture( GL_TEXTURE_CUBE_MAP, cubemapTexture );
        glDrawArrays( GL_TRIANGLES, 0, 36 );
        glBindVertexArray( 0 );
        glDepthFunc( GL_LESS );
        
//        ImGuiWindowing();
        glfwSwapBuffers( window );
    }
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    
    glfwTerminate( );
    return 0;
}

void DoMovement( )
{
    // CAMERA CONTROLS
    if ( keys[GLFW_KEY_UP] )
    {
        camera.ProcessKeyboard( FORWARD, deltaTime );
    }

    if ( keys[GLFW_KEY_DOWN])
    {
        camera.ProcessKeyboard( BACKWARD, deltaTime );
    }

    if ( keys[GLFW_KEY_LEFT] )
    {
        camera.ProcessKeyboard( LEFT, deltaTime );
    }

    if ( keys[GLFW_KEY_RIGHT] )
    {
        camera.ProcessKeyboard( RIGHT, deltaTime );
    }
    
    // BALL CONTROLS
    if ( keys[GLFW_KEY_W] )
    {
        startingPos.y += movementSpeed;
    }

    if ( keys[GLFW_KEY_A])
    {
        startingPos.x -= movementSpeed;
    }

    if ( keys[GLFW_KEY_S] )
    {
        startingPos.y -= movementSpeed;
    }

    if ( keys[GLFW_KEY_D] )
    {
        startingPos.x += movementSpeed;
    }
}

void KeyCallback( GLFWwindow *window, int key, int scancode, int action, int mode )
{
    if ( GLFW_KEY_ESCAPE == key && GLFW_PRESS == action )
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    
    if ( key >= 0 && key < 1024 )
    {
        if ( action == GLFW_PRESS )
        {
            keys[key] = true;
        }
        else if ( action == GLFW_RELEASE )
        {
            keys[key] = false;
        }
    }
}

void MouseCallback( GLFWwindow *window, double xPos, double yPos )
{
    if ( firstMouse )
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }
    
    GLfloat xOffset = xPos - lastX;
    GLfloat yOffset = lastY - yPos;  // Reversed since y-coordinates go from bottom to left
    
    lastX = xPos;
    lastY = yPos;
    
    camera.ProcessMouseMovement( xOffset, yOffset );
}


void ImGuiInit() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImFontAtlas* atlas = io.Fonts;
    ImFontConfig* config = new ImFontConfig;
    config->GlyphRanges = atlas->GetGlyphRangesDefault();
    config->PixelSnapH = true;
    //atlas->AddFontFromFileTTF("./GameResources/Fonts/CascadiaMono.ttf", 14, config);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(IMGUI_GLSL_VERSION);
}

void ImGuiWindowing() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("ImGui Debugger");
    if (ImGui::CollapsingHeader("Global Light")) {
        if (ImGui::TreeNode("Camera")) {
            if (ImGui::BeginTable("ColorAttributes_1", 1))
            {
                ImGui::TableNextColumn();
                ImGui::SliderFloat("Camera X", &CamX, -10, 10);
                ImGui::TableNextColumn();
                ImGui::SliderFloat("Camera Y", &CamY, -10, 10);
                ImGui::TableNextColumn();
                ImGui::SliderFloat("Camera Z", &CamZ, -10, 10);
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
    }

 
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

