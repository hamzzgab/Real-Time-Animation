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
#include "maths_funcs.h"

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

// ROTATIONS
glm::vec3 RotationAxis = glm::vec3(0.0f, 0.0f, 0.0f);

bool yawPress = false;
bool pitchPress = false;
bool rollPress = false;

bool gridOn = true;

// Camera
GLfloat CamX = 0.0f;
GLfloat CamY = 0.0f;
GLfloat CamZ = 3.0f;

Camera camera( glm::vec3( CamX, CamY, CamZ ) );
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLfloat movementSpeed = 0.5f;
glm::vec3 QuaternionAxis = glm::vec3(0.0f, 0.0f, 0.0f);

glm::mat4 PlaneModel     = glm::mat4(1.0f);
glm::mat4 PropellerModel = glm::mat4(1.0f);
glm::mat4 YawModel       = glm::mat4(1.0f);
glm::mat4 PitchModel = glm::mat4(1.0f);
glm::mat4 RollModel = glm::mat4(1.0f);

glm::mat4 PlanePitch = glm::mat4(1.0f);
glm::mat4 PlaneYaw = glm::mat4(1.0f);
glm::mat4 PlaneRoll = glm::mat4(1.0f);

glm::mat4 planeMatrix = glm::mat4(1.0f);

glm::quat planeOGQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

bool quaternion_set = false;
bool euler_set = true;

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

static GLFWwindow *window = nullptr;
int main( )
{

    // Init GLFW
    glfwInit( );
    // Set all the required options for GLFW
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );
    
    // Create a GLFWwindow object that we can use for GLFW's functions
    window = glfwCreateWindow( WIDTH, HEIGHT, "Reflectance Models", nullptr, nullptr );
    
    if ( nullptr == window )
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate( );
        
        return EXIT_FAILURE;
    }
    
    glfwMakeContextCurrent( window );
    
    glfwGetFramebufferSize( window, &SCREEN_WIDTH, &SCREEN_HEIGHT );
    
    // Set the required callback functions
    glfwSetKeyCallback( window, KeyCallback );
    glfwSetCursorPosCallback( window, MouseCallback );
    glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
    
    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
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
    Model Plane( "res/models/plane.obj" );
    Model Propeller( "res/models/propeller.obj" );
    Model Circle( "res/models/Circle.obj" );

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
    
    // compile and setup the shader
    // ----------------------------
    Shader textShader("res/shaders/text.vs", "res/shaders/text.frag");
    textShader.Use();
    glm::mat4 projection_text = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(textShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection_text));
    
    Text text;
    
    // Game loop
    while( !glfwWindowShouldClose( window ) )
    {
        yawPress = false;
        pitchPress = false;
        rollPress = false;
        
        GLfloat textSizeYaw = 0.5f;
        GLfloat textSizePitch = 0.5f;
        GLfloat textSizeRoll = 0.5f;
        
        GLfloat amtIncr = 0.1f;
        GLfloat scaleGrid = 0.5f;

        // Set frame time
        GLfloat currentFrame = glfwGetTime( );
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Check and call events
        glfwPollEvents( );
        DoMovement( );
        
        // Clear the colorbuffer
        glClearColor(0.5f, 0.5f, 0.5f, 0.1f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        
        glm::mat4 view = camera.GetViewMatrix( );
        
        glm::mat4 PlaneModel = glm::mat4(1.0f);
        glm::mat4 PropellerModel = glm::mat4(1.0f);
        glm::mat4 YawModel = glm::mat4(1.0f);
        glm::mat4 PitchModel = glm::mat4(1.0f);
        glm::mat4 RollModel = glm::mat4(1.0f);
        
        blinnPhongShader.Use( );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "projection" ), 1, GL_FALSE, glm::value_ptr( projection ) );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "view" ), 1, GL_FALSE, glm::value_ptr( view ) );
        
        /*
        PITCH  - X
        YAW    - Y
        ROTATE - Z
        */
        
        PlaneModel = glm::mat4(1.0f);
        PlaneModel = glm::scale(PlaneModel, glm::vec3(0.2));
        
        glm::mat4 rotationMatrix = glm::toMat4(planeOGQuat);

        if (quaternion_set)
        {
            PlaneModel = PlaneModel * rotationMatrix;
            gridOn = false;
        }
        
        if ( euler_set )
        {
            PlaneModel = glm::rotate(PlaneModel, glm::radians(RotationAxis.y), glm::vec3(0.0f, 1.0f, 0.0f));
            PlaneYaw = PlaneModel;
            
            PlaneModel = glm::rotate(PlaneModel, glm::radians(RotationAxis.z), glm::vec3(0.0f, 0.0f, 1.0f));
            PlaneRoll = PlaneModel;
            
            PlaneModel = glm::rotate(PlaneModel, glm::radians(RotationAxis.x), glm::vec3(1.0f, 0.0f, 0.0f));
            PlanePitch = PlaneModel;
        }

        PropellerModel = glm::mat4(1.0f);
        PropellerModel = glm::translate( PropellerModel, glm::vec3( 0.0f, 0.0f, 3.6f ) );
        PropellerModel = PlaneModel * PropellerModel;
        PropellerModel = glm::rotate(PropellerModel, (GLfloat)glfwGetTime()*100.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        
        if (gridOn)
        {
            
            if ( euler_set )
            {
                scaleGrid = 0.5f;
            }
            else if ( quaternion_set )
            {
                scaleGrid = 0.1f;
            }
            
            YawModel *= PlaneYaw;
            YawModel = glm::scale(YawModel, glm::vec3(scaleGrid));
            YawModel = glm::rotate(YawModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            
            PitchModel *= PlanePitch;
            PitchModel = glm::scale(PitchModel, glm::vec3(scaleGrid));
            
            RollModel *= PlaneRoll;
            RollModel = glm::scale(RollModel, glm::vec3(scaleGrid));
            RollModel = glm::rotate(RollModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            
            glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 0 );
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( YawModel ) );
            Circle.Draw( blinnPhongShader );
            
            if (yawPress){
                glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 0.0f, 1.0f, 1.0f );
                textSizeYaw += amtIncr;
            }else{
                glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 0.0f, 1.0f, 0.3f );
            }
            
            glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 0 );
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( PitchModel ) );
            Circle.Draw( blinnPhongShader );
            
            if (pitchPress){
                glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 1.0f, 0.0f, 1.0f );
                textSizePitch += amtIncr;
            }else{
                glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 1.0f, 0.0f, 0.3f );
            }
            
            glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 0 );
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( RollModel ) );
            Circle.Draw( blinnPhongShader );
            
            if (rollPress){
                glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 1.0f, 0.0f, 0.0f, 1.0f );
                textSizeRoll += amtIncr;
            }else{
                glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 1.0f, 0.0f, 0.0f, 0.3f );
            }
        }else{
            if (yawPress){
                textSizeYaw += amtIncr;
            }
            if (pitchPress){
                textSizePitch += amtIncr;
            }
            if (rollPress){
                textSizeRoll += amtIncr;
            }
        }

        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( PropellerModel ) );
        Propeller.Draw( blinnPhongShader );
        
        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( PlaneModel ) );
        Plane.Draw( blinnPhongShader );
        
        blinnPhongLighting( blinnPhongShader );
        
        if ( euler_set )
        {
            text.RenderText(textShader, "Euler Rotations", 325.0f, 550.0f, textSizeYaw, glm::vec3(0.0f, 0.0f, 0.0f));
        }
        else if ( quaternion_set )
        {
            text.RenderText(textShader, "Quaternion Rotations", 255.0f, 550.0f, textSizeYaw, glm::vec3(0.0f, 0.0f, 0.0f));
        }
        
        text.RenderText(textShader, "Y - Yaw", 10.0f, 80.0f, textSizeYaw, glm::vec3(0.0f, 0.0f, 1.0f));
        text.RenderText(textShader, "P - Pitch", 10.0f, 50.0f, textSizePitch, glm::vec3(0.0f, 1.0f, 0.0f));
        text.RenderText(textShader, "R - Roll", 10.0f, 20.0f, textSizeRoll, glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Draw skybox as last
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
        
        ImGuiWindowing();
        // Swap the buffers
        glfwSwapBuffers( window );
    }
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    
    glfwTerminate( );
    return 0;
}

void DoMovement( )
{
//     Camera controls
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
    
    /*
    PITCH  - X
    YAW    - Y
    ROTATE - Z
    */
    
    // YAW
    if ( keys[GLFW_KEY_Y] ||  keys[GLFW_KEY_D])
    {
        RotationAxis.y += movementSpeed;
        
        QuaternionAxis.y += movementSpeed;
        glm::quat quatY = glm::quat(glm::cos(glm::radians(movementSpeed / 2.0f)), glm::vec3(0.0f, 1.0f, 0.0f) * glm::sin(glm::radians(movementSpeed) / 2.0f));
        planeOGQuat = planeOGQuat * quatY;
        
        yawPress = true;
        pitchPress = false;
        rollPress = false;
    }
    
    if ( keys[GLFW_KEY_A])
    {
        RotationAxis.y -= movementSpeed;
        
        QuaternionAxis.y -= movementSpeed;
        glm::quat quatY = glm::quat(glm::cos(glm::radians(-movementSpeed / 2.0f)), glm::vec3(0.0f, 1.0f, 0.0f) * glm::sin(glm::radians(-movementSpeed) / 2.0f));
        planeOGQuat = planeOGQuat * quatY;
        
        yawPress = true;
        pitchPress = false;
        rollPress = false;
    }
    
    // PITCH
    if ( keys[GLFW_KEY_P] || keys[GLFW_KEY_S] )
    {
        RotationAxis.x += movementSpeed;
        
        QuaternionAxis.x += 20.0f;
        glm::quat quatX = glm::quat(glm::cos(glm::radians(movementSpeed / 2.0f)), glm::vec3(1.0f, 0.0f, 0.0f) * glm::sin(glm::radians(movementSpeed) / 2.0f));
        planeOGQuat = planeOGQuat * quatX;
        
        yawPress = false;
        pitchPress = true;
        rollPress = false;
    }
    
    if ( keys[GLFW_KEY_W] )
    {
        RotationAxis.x -= movementSpeed;
        
        QuaternionAxis.x -= movementSpeed;
        glm::quat quatX = glm::quat(glm::cos(glm::radians(-movementSpeed / 2.0f)), glm::vec3(1.0f, 0.0f, 0.0f) * glm::sin(glm::radians(-movementSpeed) / 2.0f));
        planeOGQuat = planeOGQuat * quatX;
        
        yawPress = false;
        pitchPress = true;
        rollPress = false;
    }

    // ROLL
    if ( keys[GLFW_KEY_R] ||  keys[GLFW_KEY_Q])
    {
        RotationAxis.z += movementSpeed;
        
        QuaternionAxis.z += movementSpeed;
        glm::quat quatZ = glm::quat(glm::cos(glm::radians(movementSpeed / 2.0f)), glm::vec3(0.0f, 0.0f, 1.0f) * glm::sin(glm::radians(movementSpeed) / 2.0f));
        planeOGQuat = planeOGQuat * quatZ;
        
        yawPress = false;
        pitchPress = false;
        rollPress = true;
    }
    
    if ( keys[GLFW_KEY_E])
    {
        RotationAxis.z -= movementSpeed;
        
        QuaternionAxis.z += movementSpeed;
        glm::quat quatZ = glm::quat(glm::cos(glm::radians(-movementSpeed / 2.0f)), glm::vec3(0.0f, 0.0f, 1.0f) * glm::sin(glm::radians(-movementSpeed) / 2.0f));
        planeOGQuat = planeOGQuat * quatZ;
        
        yawPress = false;
        pitchPress = false;
        rollPress = true;
    }

    if ( keys[GLFW_KEY_O] )
    {
        RotationAxis.x = 0.0f;
        RotationAxis.y = 0.0f;
        RotationAxis.z = 0.0f;
        
        QuaternionAxis.x = 0.0f;
        QuaternionAxis.y = 0.0f;
        QuaternionAxis.z = 0.0f;
    }
    
    if ( keys[GLFW_KEY_F] )
    {
        camera.FPP();
    }
    
    if ( keys[GLFW_KEY_T] )
    {
        camera.TPP();
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
    
    if ( action == GLFW_PRESS){
        if ( keys[GLFW_KEY_G] )
        {
            gridOn = !gridOn;
        }
        
        if ( keys[GLFW_KEY_1] )
        {
            euler_set = true;
            quaternion_set = false;
        }
        
        if ( keys[GLFW_KEY_2] )
        {
            euler_set = false;
            quaternion_set = true;
        }
        
        /*
        if ( keys [GLFW_KEY_Z] )
        {
            QuaternionAxis.x += 30 * deltaTime;
            yawPress = false;
            pitchPress = true;
            rollPress = false;
        }
        
        if ( keys [GLFW_KEY_X] )
        {
            QuaternionAxis.y += 30 * deltaTime;
            yawPress = true;
            pitchPress = false;
            rollPress = false;
        }
        
        if ( keys [GLFW_KEY_C] )
        {
            QuaternionAxis.z += 30 * deltaTime;
            yawPress = false;
            pitchPress = false;
            rollPress = true;
        }

        if ( keys [GLFW_KEY_V] )
        {
            QuaternionAxis.x -= 30 * deltaTime;
            yawPress = false;
            pitchPress = true;
            rollPress = false;
        }
        
        if ( keys [GLFW_KEY_B] )
        {
            QuaternionAxis.y -= 30 * deltaTime;
            yawPress = true;
            pitchPress = false;
            rollPress = false;
        }
        
        if ( keys [GLFW_KEY_N] )
        {
            QuaternionAxis.z -= 30 * deltaTime;
            yawPress = false;
            pitchPress = false;
            rollPress = true;
        }
         */
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
        if (ImGui::TreeNode("Rotation")) {
            if (ImGui::BeginTable("ColorAttributes_1", 1))
            {
                ImGui::TableNextColumn();
                ImGui::InputFloat("Pitch Degree", &QuaternionAxis.x);
                ImGui::TableNextColumn();
                ImGui::InputFloat("Yaw Degree", &QuaternionAxis.y);
                ImGui::TableNextColumn();
                ImGui::InputFloat("Roll Degree", &QuaternionAxis.z);
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
        
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

