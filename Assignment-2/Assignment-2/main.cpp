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
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/euler_angles.hpp>

// Other Libs
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "SOIL2/SOIL2.h"

// Text Rendering
#include <ft2build.h>
#include FT_FREETYPE_H


#define IMGUI_GLSL_VERSION      "#version 330 core"



//------------SCREEN PROPERTIES---------------------------------------
const GLuint WIDTH = 1600, HEIGHT = 1000;
//const GLuint WIDTH = 800, HEIGHT = 800;
int SCREEN_WIDTH, SCREEN_HEIGHT;
//--------------------------------------------------------------------

//------------FUNCTION PROTOTYPES-------------------------------------
void KeyCallback( GLFWwindow *window, int key, int scancode, int action, int mode );
void MouseCallback( GLFWwindow *window, double xPos, double yPos );
void DoMovement( );
//--------------------------------------------------------------------

void ImGuiInit();
void ImGuiWindowing();

//------------CAMERA COMPONENTS---------------------------------------
GLfloat CamX = 0.0f;
GLfloat CamY = 0.0f;
GLfloat CamZ = 10.0f;
Camera camera( glm::vec3( CamX, CamY, CamZ ) );
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
//--------------------------------------------------------------------

//------------END TARGET COMPONENTS-----------------------------------
GLfloat movementSpeed = 0.075f;
glm::vec3 BallPosition(10.0f, 0.0f, 0.0f);
//--------------------------------------------------------------------

//------------ANALYTICAL VARIABLES------------------------------------
float upperArmAngle;
float lowerArmAngle;
int armLengthL1 = 2;
int armLengthL2 = 2;
bool forwardKinematics = false;
GLfloat forwardRotDegUpper1 = 0.0f;
GLfloat forwardRotDegLower1 = 0.0f;

GLfloat forwardRotDegUpper2 = 0.0f;
GLfloat forwardRotDegLower2 = 0.0f;

GLfloat forwardRotDegHand1  = 0.0f;
GLfloat forwardRotDegHand2  = 0.0f;
//--------------------------------------------------------------------

//------------CCD VARIABLES-------------------------------------------
int EFFECTOR_POS      = 3;
GLfloat IK_POS_THRESH = 1.0f;
bool m_Damping        = false;
bool m_DOF_Restrict   = false;
int MAX_IK_TRIES      = 100;



glm::quat quatRotations[3];
glm::vec3 rotationDirections[3];
glm::vec3 links[3];
GLfloat linksDampWidth[3];
GLint linksDOFRestrictionsMin[3];
GLint linksDOFRestrictionsMax[3];
GLfloat linkTurnAngles[3];
glm::vec3 linkTurnEulAngles[3] {};
glm::mat4 endEffector = glm::mat4(1.0f);
//--------------------------------------------------------------------

bool SetAnalytical = true;
bool SetNumerical = false;

void setUpLinks() {
    links[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    links[1] = glm::vec3(2.0f, 0.0f, 0.0f);
    links[2] = glm::vec3(4.0f, 0.0f, 0.0f);
    
    linksDampWidth[0] = 30.0f;
    linksDampWidth[1] = 30.0f;
    linksDampWidth[2] = 30.0f;
    
    linksDOFRestrictionsMin[0] = -50;
    linksDOFRestrictionsMin[1] = -50;
    linksDOFRestrictionsMin[2] = -50;
    
    linksDOFRestrictionsMax[0] = 50;
    linksDOFRestrictionsMax[1] = 50;
    linksDOFRestrictionsMax[2] = 50;
    
    linkTurnAngles[0] = 0.0f;
    linkTurnAngles[1] = 0.0f;
    linkTurnAngles[2] = 0.0f;
}

//--------------------------------------------------------------------
// Procedure: BlinnPhongLighting
// Purpose:   To add lighting in a scene to brighten it up
// Arguments: Shader to be used to light up the scene
// Returns:   None
//--------------------------------------------------------------------
void BlinnPhongLighting(Shader shader){
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


//--------------------------------------------------------------------
// Procedure: ComputeAnalyticalLink
// Purpose:   Compute an IK Solution to an end effector position
// Arguments: End Target (x,y)
// Returns:   None
//--------------------------------------------------------------------
void ComputeAnalyticalLink(float x, float y) {
    //-----LOCAL VARIABLES------------------------------------------------
    float l1 = 0, l2 = 0;
    float ex = 0, ey = 0;
    float cosAngle = 0;
    //--------------------------------------------------------------------
    
    ex = x;
    ey = y;
    
    float d = sqrt((x*x) + (y*y));
    
    l1 = armLengthL1;
    l2 = armLengthL2;
    
    float cosThetaT = y / x;
    float ThetaT = (float)glm::atan(cosThetaT);

    float cosTheta1MinusThetaT = (l1*l1 + ex*ex + ey*ey - l2*l2) / (2 * l1 * (d));
    float Theta1 = (float)glm::acos(cosTheta1MinusThetaT) + ThetaT;

    float Theta2 = (float)glm::acos(((x*x + y*y) - (l1*l1 + l2*l2)) / (2 * l1 * l2));
    
    cosAngle = ((ex * ex) + (ey * ey) - (l1 * l1) - (l2 * l2)) / (2 * l1 * l2);

    if (cosAngle >= -1.0 && cosAngle <= 1.0) {
        lowerArmAngle = -glm::degrees(Theta2);
        upperArmAngle = glm::degrees(Theta1);
    }
    
}

glm::vec3 GetWorldPosition(glm::mat4& transform) {
    return glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
}
//--------------------------------------------------------------------
// Procedure: ComputeCCDLink
// Purpose:   Compute an IK Solution to an end effector position
// Arguments: End Target (x,y)
// Returns:   None
//--------------------------------------------------------------------
bool shouldDraw = true;
void ComputeCCD(glm::vec3 TargetPos)
{
    //-----LOCAL VARIABLES------------------------------------------------
    glm::vec3 rootPos,curEnd,desiredEnd,targetVector,curVector,crossResult = glm::vec3(1.0f);
    double cosAngle,turnAngle,turnDeg;
    int link, tries;
    //--------------------------------------------------------------------

    link = EFFECTOR_POS - 1;
    tries = 0;

    do
    {
        rootPos = links[link];
        curEnd = GetWorldPosition(endEffector);
        desiredEnd = TargetPos;
        if (glm::distance(curEnd, desiredEnd) > IK_POS_THRESH)
        {
            shouldDraw = false;
            curVector = curEnd - rootPos;
            targetVector = TargetPos - rootPos;
            
            curVector    = glm::normalize(curVector);
            targetVector = glm::normalize(targetVector);
            cosAngle     = glm::dot(targetVector, curVector);
            
            if (cosAngle < 0.99999f)
            {
                crossResult = glm::cross(curVector, targetVector);
                rotationDirections[link] = crossResult;
                
                turnAngle = glm::acos((GLfloat)cosAngle);
                turnDeg = glm::degrees(turnAngle);
                
                if (crossResult.z > 0.0f)
                {

                    if (m_Damping && turnDeg > linksDampWidth[link])
                    {
                        turnDeg -= linksDampWidth[link];
                    }

                    linkTurnAngles[link] -= (GLfloat)turnDeg;

                    if (m_DOF_Restrict && linkTurnAngles[link] < (GLfloat)linksDOFRestrictionsMin[link])
                    {
                        linkTurnAngles[link] -= (GLfloat)linksDOFRestrictionsMin[link];
                    }
                }
                else if (crossResult.z < 0.0f)
                {

                    if (m_Damping && turnDeg > linksDampWidth[link])
                    {
                        turnDeg += linksDampWidth[link];
                    }
                    linkTurnAngles[link] += (GLfloat)turnDeg;

                    if (m_DOF_Restrict && linkTurnAngles[link] > (GLfloat)linksDOFRestrictionsMax[link])
                    {
                        linkTurnAngles[link] += (GLfloat)linksDOFRestrictionsMax[link];
                    }
                }
            }
            if (--link < 0) link = EFFECTOR_POS - 1;
        }
        else {
            shouldDraw = true;
        }
    }while(tries++ < MAX_IK_TRIES );

     
}

static GLFWwindow *window = nullptr;
int main()
{
    setUpLinks();
    glfwInit( );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );
    window = glfwCreateWindow( WIDTH, HEIGHT, "Hierarchical Creatures", nullptr, nullptr );
    
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
    Model Face( "res/models/Face.obj" );
    
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
        
        blinnPhongShader.Use( );
        
        if (SetAnalytical)
        {
            ComputeAnalyticalLink(BallPosition.x, BallPosition.y);
            
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "projection" ), 1, GL_FALSE, glm::value_ptr( projection ) );
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "view" ), 1, GL_FALSE, glm::value_ptr( view ) );
            glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );

            
            if (!forwardKinematics)
            {
                // UPPER ARM
                glm::mat4 upperArmModel = glm::mat4(1.0f);
                upperArmModel = glm::rotate(upperArmModel, glm::radians(upperArmAngle), glm::vec3(0.0f, 0.0f, 1.0f));
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel ) );
                Limb.Draw( blinnPhongShader );

                // LOWER ARM
                glm::mat4 lowerArmModel = glm::mat4(1.0f);
                lowerArmModel = glm::translate(lowerArmModel, glm::vec3(2.0f, 0.0f, 0.0f));
                lowerArmModel = glm::rotate(lowerArmModel, glm::radians(lowerArmAngle), glm::vec3(0.0f, 0.0f, 1.0f));
                lowerArmModel = upperArmModel * lowerArmModel;

                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel ) );
                Limb.Draw( blinnPhongShader );
            }else{
                // UPPER ARM
                glm::mat4 upperArmModel = glm::mat4(1.0f);
                upperArmModel = glm::rotate(upperArmModel, glm::radians(forwardRotDegUpper1), glm::vec3(0.0f, 0.0f, 1.0f));
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel ) );
                Limb.Draw( blinnPhongShader );

                // LOWER ARM
                glm::mat4 lowerArmModel = glm::mat4(1.0f);
                lowerArmModel = glm::translate(lowerArmModel, glm::vec3(2.0f, 0.0f, 0.0f));
                lowerArmModel = glm::rotate(lowerArmModel, glm::radians(forwardRotDegLower1), glm::vec3(0.0f, 0.0f, 1.0f));
                lowerArmModel = upperArmModel * lowerArmModel;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel ) );
                Limb.Draw( blinnPhongShader );
   
                glm::mat4 upperArmModel2 = glm::mat4(1.0f);
                upperArmModel2 = glm::translate(upperArmModel2, glm::vec3(-2.0f, 0.0f, 0.0f));
                upperArmModel2 = glm::rotate(upperArmModel2, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                upperArmModel2 = glm::rotate(upperArmModel2, glm::radians(forwardRotDegUpper2), glm::vec3(0.0f, 0.0f, 1.0f));
                
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel2 ) );
                Limb.Draw( blinnPhongShader );

                // LOWER ARM
                glm::mat4 lowerArmModel2 = glm::mat4(1.0f);
                lowerArmModel2 = glm::translate(lowerArmModel2, glm::vec3(2.0f, 0.0f, 0.0f));
                lowerArmModel2 = glm::rotate(lowerArmModel2, glm::radians(forwardRotDegLower2), glm::vec3(0.0f, 0.0f, 1.0f));
                lowerArmModel2 = upperArmModel2 * lowerArmModel2;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel2 ) );
                Limb.Draw( blinnPhongShader );
   
                
                // BALL
                glm::mat4 BallModel = glm::mat4(1.0f);
                BallModel = glm::translate(BallModel, glm::vec3(2.0f, 0.0f, 0.0f));
                BallModel = lowerArmModel * BallModel;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( BallModel ) );
                Ball.Draw( blinnPhongShader );
            }

        }else if (SetNumerical)
        {
            ComputeCCD(BallPosition);
            
            if (!forwardKinematics)
            {
                // UPPER ARM
                glm::mat4 upperArmModel = glm::mat4(1.0f);
                upperArmModel = glm::rotate(upperArmModel, glm::radians(linkTurnAngles[0]), glm::vec3(0.0f, 0.0f, 1.0f));
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel ) );
                Limb.Draw( blinnPhongShader );

                // Lower Arm
                glm::mat4 lowerArmModel = glm::mat4(1.0f);
                lowerArmModel = glm::translate(lowerArmModel, glm::vec3(2.0f, 0.0f, 0.0f));
                lowerArmModel = glm::rotate(lowerArmModel, glm::radians(linkTurnAngles[1]), glm::vec3(0.0f, 0.0f, 1.0f));
                lowerArmModel *= glm::toMat4(quatRotations[1]);
                lowerArmModel = upperArmModel * lowerArmModel;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel ) );
                Limb.Draw( blinnPhongShader );

                // HAND
                glm::mat4 handModel = glm::mat4(1.0f);
                handModel = glm::translate(handModel, glm::vec3(2.0f, 0.0f, 0.0f));
                handModel = glm::rotate(handModel, glm::radians(linkTurnAngles[2]), glm::vec3(0.0f, 0.0f, 1.0f));
                handModel = lowerArmModel * handModel;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( handModel ) );
                Limb.Draw( blinnPhongShader );
                
                endEffector = glm::mat4(1.0f);
                endEffector = glm::translate(endEffector, glm::vec3(2.0f, 0.0f, 0.0f));
                endEffector = handModel * endEffector;
            }else{
                // UPPER ARM
                glm::mat4 upperArmModel = glm::mat4(1.0f);
                upperArmModel = glm::rotate(upperArmModel, glm::radians(forwardRotDegUpper1), glm::vec3(0.0f, 0.0f, 1.0f));
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel ) );
                Limb.Draw( blinnPhongShader );

                // Lower Arm
                glm::mat4 lowerArmModel = glm::mat4(1.0f);
                lowerArmModel = glm::translate(lowerArmModel, glm::vec3(2.0f, 0.0f, 0.0f));
                lowerArmModel = glm::rotate(lowerArmModel, glm::radians(forwardRotDegLower1), glm::vec3(0.0f, 0.0f, 1.0f));
                lowerArmModel *= glm::toMat4(quatRotations[1]);
                lowerArmModel = upperArmModel * lowerArmModel;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel ) );
                Limb.Draw( blinnPhongShader );

                // HAND
                glm::mat4 handModel = glm::mat4(1.0f);
                handModel = glm::translate(handModel, glm::vec3(2.0f, 0.0f, 0.0f));
                handModel = glm::rotate(handModel, glm::radians(forwardRotDegHand1), glm::vec3(0.0f, 0.0f, 1.0f));
                handModel = lowerArmModel * handModel;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( handModel ) );
                Limb.Draw( blinnPhongShader );
                
                // UPPER ARM
                glm::mat4 upperArmModel1 = glm::mat4(1.0f);
                upperArmModel1 = glm::translate(upperArmModel1, glm::vec3(-2.0f, 0.0f, 0.0f));
                upperArmModel1 = glm::rotate(upperArmModel1, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                upperArmModel1 = glm::rotate(upperArmModel1, glm::radians(forwardRotDegUpper2), glm::vec3(0.0f, 0.0f, 1.0f));
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( upperArmModel1 ) );
                Limb.Draw( blinnPhongShader );

                // Lower Arm
                glm::mat4 lowerArmModel1 = glm::mat4(1.0f);
                lowerArmModel1 = glm::translate(lowerArmModel1, glm::vec3(2.0f, 0.0f, 0.0f));
                lowerArmModel1 = glm::rotate(lowerArmModel1, glm::radians(forwardRotDegLower2), glm::vec3(0.0f, 0.0f, 1.0f));
                lowerArmModel1 *= glm::toMat4(quatRotations[1]);
                lowerArmModel1 = upperArmModel1 * lowerArmModel1;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( lowerArmModel1 ) );
                Limb.Draw( blinnPhongShader );

                // HAND
                glm::mat4 handModel1 = glm::mat4(1.0f);
                handModel1 = glm::translate(handModel1, glm::vec3(2.0f, 0.0f, 0.0f));
                handModel1 = glm::rotate(handModel1, glm::radians(forwardRotDegHand2), glm::vec3(0.0f, 0.0f, 1.0f));
                handModel1 = lowerArmModel1 * handModel1;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( handModel1 ) );
                Limb.Draw( blinnPhongShader );
                
                // BALL
                glm::mat4 BallModel = glm::mat4(1.0f);
                BallModel = glm::translate(BallModel, glm::vec3(2.0f, 0.0f, 0.0f));
                BallModel = handModel * BallModel;
                glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( BallModel ) );
                Ball.Draw( blinnPhongShader );
            }
        }
        
        glm::mat4 FaceModel = glm::mat4(1.0f);
        FaceModel = glm::scale(FaceModel, glm::vec3(0.75f));
        FaceModel = glm::translate(FaceModel, glm::vec3(-1.5f, 3.5f, 0.0f));
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( FaceModel ) );
        Face.Draw( blinnPhongShader );
        BlinnPhongLighting( blinnPhongShader );
        
        glm::mat4 BodyModel = glm::mat4(1.0f);
        BodyModel = glm::translate(BodyModel, glm::vec3(-1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( BodyModel ) );
        Body.Draw( blinnPhongShader );
        BlinnPhongLighting( blinnPhongShader );
        
        if (!forwardKinematics)
        {
            glm::mat4 BallModel = glm::mat4(1.0f);
            BallModel = glm::translate(BallModel, BallPosition);
            blinnPhongShader.Use( );
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "projection" ), 1, GL_FALSE, glm::value_ptr( projection ) );
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "view" ), 1, GL_FALSE, glm::value_ptr( view ) );
            glUniform1i( glGetUniformLocation( blinnPhongShader.Program, "useIt" ), 1 );
            glUniformMatrix4fv( glGetUniformLocation( blinnPhongShader.Program, "model" ), 1, GL_FALSE, glm::value_ptr( BallModel ) );
            Ball.Draw( blinnPhongShader );
            BlinnPhongLighting( blinnPhongShader );
        }

        
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
    if ( keys[GLFW_KEY_W] )
    {
        camera.ProcessKeyboard( FORWARD, deltaTime );
    }
    
    if ( keys[GLFW_KEY_S])
    {
        camera.ProcessKeyboard( BACKWARD, deltaTime );
    }
    
    if ( keys[GLFW_KEY_A] )
    {
        camera.ProcessKeyboard( LEFT, deltaTime );
    }
    
    if ( keys[GLFW_KEY_D] )
    {
        camera.ProcessKeyboard( RIGHT, deltaTime );
    }
    
    // BALL CONTROLS
    if ( keys[GLFW_KEY_UP] )
    {
        BallPosition.y += movementSpeed;
    }
    
    if ( keys[GLFW_KEY_LEFT])
    {
        BallPosition.x -= movementSpeed;
    }
    
    if ( keys[GLFW_KEY_DOWN] )
    {
        BallPosition.y -= movementSpeed;
    }
    
    if ( keys[GLFW_KEY_RIGHT] )
    {
        BallPosition.x += movementSpeed;
    }
    
    if ( keys[GLFW_KEY_Q] )
    {
        BallPosition.z -= movementSpeed;
    }
    
    if ( keys[GLFW_KEY_E] )
    {
        BallPosition.z += movementSpeed;
    }
    
    if (forwardKinematics)
    {
        if ( keys[GLFW_KEY_I] )
        {
            forwardRotDegUpper1 += 0.75;
        }

        if ( keys[GLFW_KEY_O] )
        {
            forwardRotDegUpper1 -= 0.75;
        }
        
        if ( keys[GLFW_KEY_J] )
        {
            forwardRotDegLower1 += 0.75;
        }

        if ( keys[GLFW_KEY_K] )
        {
            forwardRotDegLower1 -= 0.75;
        }
        
        if ( keys[GLFW_KEY_N] )
        {
            forwardRotDegHand1 += 0.75;
        }

        if ( keys[GLFW_KEY_M] )
        {
            forwardRotDegHand1 -= 0.75;
        }
        
        if ( keys[GLFW_KEY_Y] )
        {
            forwardRotDegUpper2 += 0.75;
        }

        if ( keys[GLFW_KEY_U] )
        {
            forwardRotDegUpper2 -= 0.75;
        }
        
        if ( keys[GLFW_KEY_G] )
        {
            forwardRotDegLower2 += 0.75;
        }

        if ( keys[GLFW_KEY_H] )
        {
            forwardRotDegLower2 -= 0.75;
        }
        
        if ( keys[GLFW_KEY_V] )
        {
            forwardRotDegHand2 += 0.75;
        }

        if ( keys[GLFW_KEY_B] )
        {
            forwardRotDegHand2 -= 0.75;
        }
    }
    
    if ( keys[GLFW_KEY_C] )
    {
        ComputeCCD(BallPosition);
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
    
    if (action == GLFW_PRESS)
    {
        if (keys[GLFW_KEY_1])
        {
            BallPosition = glm::vec3(4.0f, 0.0f, 0.0f);
            SetAnalytical = true;
            SetNumerical = false;
        }
        if (keys[GLFW_KEY_2])
        {
            BallPosition = glm::vec3(6.0f, 0.0f, 0.0f);
            SetAnalytical = false;
            SetNumerical = true;
        }
        
        if ( keys[GLFW_KEY_F] )
        {
            forwardKinematics = !forwardKinematics;
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


