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

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
const GLuint WIDTH = 800, HEIGHT = 800;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Function prototypes
void KeyCallback( GLFWwindow *window, int key, int scancode, int action, int mode );
void MouseCallback( GLFWwindow *window, double xPos, double yPos );
void DoMovement( );

void ImGuiInit();
void ImGuiWindowing();

// PROPS
GLfloat yawDeg = 0.0f;
GLfloat pitchDeg = 0.0f;
GLfloat rollDeg = 0.0f;

bool yawPress = false;
bool pitchPress = false;
bool rollPress = false;

// Camera
Camera camera( glm::vec3( 0.0f, 0.0f, 3.0f ) );
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

bool yaw_set = false;

void blinnPhongLighting(Shader shader){
    GLint lightDirLoc = glGetUniformLocation( shader.Program, "light.direction" );
    GLint viewPosLoc  = glGetUniformLocation( shader.Program, "viewPos" );
    
    glUniform3f( lightDirLoc, -5.0f, 0.5f, -1.5f);
    glUniform3f( viewPosLoc,  camera.GetPosition( ).x, camera.GetPosition( ).y, camera.GetPosition( ).z );
    
    // LIGHT PROPERTIES
    glUniform3f( glGetUniformLocation( shader.Program, "light.ambient" ), 0.5f, 0.5f, 0.5f );
    glUniform3f( glGetUniformLocation( shader.Program, "light.diffuse" ), 0.5f, 0.5f, 0.5f );
    glUniform3f( glGetUniformLocation( shader.Program, "light.specular" ), 1.0f, 1.0f, 1.0f );

    glUniform1i( glGetUniformLocation( shader.Program, "blinn" ), false ? 1 : 0 );
    
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
//    glfwSetCursorPosCallback( window, MouseCallback );
//    glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
    
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
    
    // Setup and compile our shaders
    Shader blinnPhongShader( "res/shaders/blinnPhonVS.vs", "res/shaders/blinnPhonFS.frag" );
    
    // Load models
    Model Plane( "res/models/plane.obj" );
    Model Propeller( "res/models/propeller.obj" );
    Model Circle( "res/models/Circle.obj" );

    // Draw in wireframe
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    
    glm::mat4 projection = glm::perspective( camera.GetZoom( ), ( float )SCREEN_WIDTH/( float )SCREEN_HEIGHT, 0.1f, 100.0f );
    
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
        // Set frame time
        GLfloat currentFrame = glfwGetTime( );
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Check and call events
        glfwPollEvents( );
        DoMovement( );
        
        // Clear the colorbuffer
        glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
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
        
        PlaneModel = glm::mat4(1.0f);
        PlaneModel = glm::scale(PlaneModel, glm::vec3(0.2));
        
        PlaneModel = glm::rotate(PlaneModel, glm::radians(yawDeg), glm::vec3(0.0f, 1.0f, 0.0f));
        PlaneModel = glm::rotate(PlaneModel, glm::radians(pitchDeg), glm::vec3(1.0f, 0.0f, 0.0f));
        PlaneModel = glm::rotate(PlaneModel, glm::radians(rollDeg), glm::vec3(0.0f, 0.0f, 1.0f));

        PropellerModel = glm::mat4(1.0f);
        PropellerModel = glm::translate( PropellerModel, glm::vec3( 0.0f, 0.0f, 3.6f ) );
        PropellerModel = PlaneModel * PropellerModel;
        PropellerModel = glm::rotate(PropellerModel, (GLfloat)glfwGetTime()*100.0f, glm::vec3(0.0f, 0.0f, 1.0f));

        YawModel = glm::mat4(1.0f);
        YawModel = glm::scale(YawModel, glm::vec3(0.15));
        YawModel = glm::rotate(YawModel, glm::radians(yawDeg), glm::vec3(0.0f, 1.0f, 0.0f));
        
        PitchModel = glm::mat4(1.0f);
        PitchModel = glm::scale(PitchModel, glm::vec3(0.15));
        PitchModel = glm::rotate(PitchModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        PitchModel = glm::rotate(PitchModel, glm::radians(pitchDeg), glm::vec3(1.0f, 0.0f, 0.0f));

        RollModel = glm::mat4(1.0f);
        RollModel = glm::scale(RollModel, glm::vec3(0.15));
        RollModel = glm::rotate(RollModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        RollModel = glm::rotate(RollModel, glm::radians(rollDeg), glm::vec3(0.0f, 0.0f, 1.0f));

//        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 0 );
//        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( YawModel ) );
//        Circle.Draw( blinnPhongShader );
//        if (yawPress){
//            glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 0.0f, 1.0f, 1.0f );
//        }else{
//            glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 0.0f, 1.0f, 0.3f );
//        }

        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 0 );
        Circle.Draw( blinnPhongShader );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( RollModel ) );
        if (rollPress){
            glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 1.0f, 0.0f, 0.0f, 1.0f );
        }else{
            glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 1.0f, 0.0f, 0.0f, 0.3f );
        }

        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 0 );
        Circle.Draw( blinnPhongShader );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( PitchModel ) );
        if (pitchPress){
            glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 1.0f, 0.0f, 1.0f );
        }else{
            glUniform4f( glGetUniformLocation( blinnPhongShader.Program, "colorIt" ), 0.0f, 1.0f, 0.0f, 0.3f );
        }
        

        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( PropellerModel ) );
        Propeller.Draw( blinnPhongShader );
        
        glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( PlaneModel ) );
        Plane.Draw( blinnPhongShader );
        
        blinnPhongLighting( blinnPhongShader );
        
        ImGuiWindowing();
        // Swap the buffers
        glfwSwapBuffers( window );
    }
    
    glfwTerminate( );
    return 0;
}

// Moves/alters the camera positions based on user input
void DoMovement( )
{
    // Camera controls
    if ( keys[GLFW_KEY_W] || keys[GLFW_KEY_UP] )
    {
        camera.ProcessKeyboard( FORWARD, deltaTime );
    }
    
    if ( keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN] )
    {
        camera.ProcessKeyboard( BACKWARD, deltaTime );
    }
    
    if ( keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT] )
    {
        camera.ProcessKeyboard( LEFT, deltaTime );
    }

    if ( keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT] )
    {
        camera.ProcessKeyboard( RIGHT, deltaTime );
    }
    
    if ( keys[GLFW_KEY_Y] )
    {
        yawDeg += 0.5f;
        yawPress = true;
        pitchPress = false;
        rollPress = false;
    }
    
    // PITCH
    if ( keys[GLFW_KEY_P] )
    {
        pitchDeg += 0.5f;
        yawPress = false;
        pitchPress = true;
        rollPress = false;
    }

    // ROLL
    if ( keys[GLFW_KEY_R] )
    {
        rollDeg += 0.5f;
        yawPress = false;
        pitchPress = false;
        rollPress = true;
    }

    if ( keys[GLFW_KEY_O] )
    {
        yawDeg = 0.0f;
        pitchDeg = 0.0f;
        rollDeg = 0.0f;
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
        if (ImGui::TreeNode("Rotation")) {
            if (ImGui::BeginTable("ColorAttributes_1", 1))
            {
                ImGui::TableNextColumn();
                ImGui::InputFloat("Yaw Degree", &yawDeg);
                ImGui::TableNextColumn();
                ImGui::InputFloat("Pitch Degree", &pitchDeg);
                ImGui::TableNextColumn();
                ImGui::InputFloat("Roll Degree", &rollDeg);
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
    }

 
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

