/**
 * Author: Calvin
 * Assignment: Pong Clone
 * Date due: 2023-10-21, 11:59pm
 * Game idea:
 *   - pong with batteries as paddles and electric bolts as ball
 *   - each player has a charge level
 *   - moving the paddle drains the charge
 *   - hitting the bolts recharges the paddle and sends the bolt in the opposite direction
 *   - missing a bolt drains the paddle
 *   - if the paddle runs out of charge, the player loses
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 **/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

const int WINDOW_WIDTH = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char PLAYER_SPRITE_FILEPATH[] = "part_electronic_battery_modern_0.png";
const char BALL_SPRITE_FILEPATH[] = "bolt.png";

SDL_Window *g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
GLuint g_player1_texture_id;
GLuint g_player2_texture_id;
GLuint g_ball_texture_id;
glm::mat4 g_view_matrix,
    g_player1_matrix,
    g_player2_matrix,
    g_ball_matrix,
    g_projection_matrix;

float g_previous_ticks = 0.0f;

// Paddle positions on edge of screen
glm::vec3 PLAYER1_INIT_POSITION = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 PLAYER2_INIT_POSITION = glm::vec3(4.5f, 0.0f, 0.0f);

// Paddle 1
glm::vec3 g_player1_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player1_movement = glm::vec3(0.0f, 0.0f, 0.0f);
float g_player1_rotation = 0.0f;
glm::vec3 g_player1_scale = glm::vec3(0.0f, 0.0f, 0.0f);

// Paddle 2
glm::vec3 g_player2_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player2_movement = glm::vec3(0.0f, 0.0f, 0.0f);
float g_player2_rotation = 0.0f;
glm::vec3 g_player2_scale = glm::vec3(0.0f, 0.0f, 0.0f);

// Ball
glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
float g_ball_rotation = 0.0f;
glm::vec3 g_ball_scale = glm::vec3(0.0f, 0.0f, 0.0f);

bool g_player1_paddle_next = true;  // which paddle should hit the ball next

// glm::vec3 g_player_orientation = glm::vec3(0.0f, 0.0f, 0.0f);
// glm::vec3 g_player_rotation = glm::vec3(0.0f, 0.0f, 0.0f);

// How fast to move the paddles
float g_player_speed = 5.0f; // move 1 unit per second
float g_ball_speed = 2.5f;

// Charge level for paddles -
glm::uint g_player1_charge = 100;
glm::uint g_player2_charge = 100;

// ai
bool g_player2_is_ai = false;
glm::vec3 g_player2_ai_movement = glm::vec3(0.0f, 1.0f, 0.0f); // default move up

GLuint load_texture(const char *filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char *image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Textures!",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_player1_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_player2_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_player1_matrix = glm::mat4(1.0f);
    g_player2_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    // Set initial ball movement
    g_ball_movement = glm::vec3(-1.0f, 1.0f, 0.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // �������������������������������� NEW STUFF �������������������������� //
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere   //
    g_player1_movement = glm::vec3(0.0f); //
    g_player2_movement = glm::vec3(0.0f);
    //
    // �������������������������������� KEYSTROKES ������������������������� //
    //
    SDL_Event event; //
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            // Player 1 (w and s keyboard keys)
            case SDLK_w:
                g_player1_movement.y = -1.0f;
                break;
            case SDLK_s:
                g_player1_movement.y = 1.0f;
                break;
            case SDLK_a:
                g_player1_movement.x = -1.0f;
                break;
            case SDLK_d:
                g_player1_movement.x = 1.0f;
                break;

            // Player 2 (up and down arrow keys)
            case SDLK_UP:
                g_player2_movement.y = -1.0f;
                break;
            case SDLK_DOWN:
                g_player2_movement.y = 1.0f;
                break;

            // Toggle player 2 AI
            case SDLK_t:
                g_player2_is_ai = !g_player2_is_ai;
                break;

            // Quit game
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;
            default:
                break;
            }

        default:
            break;
        }
    }

    // ������������������������������� KEY HOLD ���������������������������� //
    //
    const Uint8 *key_state = SDL_GetKeyboardState(NULL); //
    // player 1
    if (key_state[SDL_SCANCODE_W])
    {
        g_player1_movement.y = 1.0f;    //
    }                                   //
    else if (key_state[SDL_SCANCODE_S]) //
    {                                   //
        g_player1_movement.y = -1.0f;   //
    }                                   //
    else if (key_state[SDL_SCANCODE_A]) //
    {                                   //
        g_player1_movement.x = -1.0f;   //
    }                                   //
    else if (key_state[SDL_SCANCODE_D]) //
    {                                   //
        g_player1_movement.x = 1.0f;    //
    }                                   //

    // player 2
    if (key_state[SDL_SCANCODE_UP])        //
    {                                      //
        g_player2_movement.y = 1.0f;       //
    }                                      //
    else if (key_state[SDL_SCANCODE_DOWN]) //
    {                                      //
        g_player2_movement.y = -1.0f;      //
    }                                      //
                                           //
    // This makes sure that the player can't "cheat" their way into moving   //
    // faster                                                                //
    if (glm::length(g_player1_movement) > 1.0f)                  //
    {                                                            //
        g_player1_movement = glm::normalize(g_player1_movement); //
    }                                                            //
    if (glm::length(g_player2_movement) > 1.0f)                  //
    {                                                            //
        g_player2_movement = glm::normalize(g_player2_movement); //
    }                                                            //
    // ��������������������������������������������������������������������� //
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks;                  // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // �������������������������������� NEW STUFF ������������������������� //
    // Add direction * units per second * elapsed time                      //
    g_player1_position += g_player1_movement * g_player_speed * delta_time; //
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;

    // Player 2 AI
    if (g_player2_is_ai)
    {
        // Move up and down
        if (g_player2_position.y >= 3.0f)
        {
            g_player2_ai_movement.y = -1.0f;
        }
        else if (g_player2_position.y <= -3.0f)
        {
            g_player2_ai_movement.y = 1.0f;
        }
        // } else {
        //     g_player2_ai_movement.y = 1.0f;
        // }
        g_player2_position += g_player2_ai_movement * g_player_speed * delta_time; //
    }
    else
    {
        // Use key presses
        g_player2_position += g_player2_movement * g_player_speed * delta_time; //
    }

    // Bound position by screen edges
    if (g_player1_position.y > 3.0f)
    {
        g_player1_position.y = 3.0f;
    }
    else if (g_player1_position.y < -3.0f)
    {
        g_player1_position.y = -3.0f;
    }

    if (g_player2_position.y > 3.0f)
    {
        g_player2_position.y = 3.0f;
    }
    else if (g_player2_position.y < -3.0f)
    {
        g_player2_position.y = -3.0f;
    }

    // Reset model matrix
    g_player1_matrix = glm::mat4(1.0f);
    g_player2_matrix = glm::mat4(1.0f);

    // Set paddle locations to edge of screen
    g_player1_matrix = glm::translate(g_player1_matrix, PLAYER1_INIT_POSITION);
    g_player2_matrix = glm::translate(g_player2_matrix, PLAYER2_INIT_POSITION);

    // Collision Detection
    /** ———— COLLISION DETECTION ———— **/
    float collision_factor = 0.40f;
    float x1_distance = fabs(g_player1_position.x + PLAYER1_INIT_POSITION.x - g_ball_position.x) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float y1_distance = fabs(g_player1_position.y + PLAYER1_INIT_POSITION.y - g_ball_position.y) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float x2_distance = fabs(g_player2_position.x + PLAYER2_INIT_POSITION.x - g_ball_position.x) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float y2_distance = fabs(g_player2_position.y + PLAYER2_INIT_POSITION.y - g_ball_position.y) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    // float x_distance = fabs(g_player1_position.x - CUP_INIT_POS.x) - ((FLOWER_INIT_SCA.x + CUP_INIT_SCA.x) / 2.0f);
    // float y_distance = fabs(g_player1_position.y - CUP_INIT_POS.y) - ((FLOWER_INIT_SCA.y + CUP_INIT_SCA.y) / 2.0f);

    if ((g_player1_paddle_next && x1_distance < 0.0f && y1_distance < 0.0f) || (!g_player1_paddle_next && x2_distance < 0.0f && y2_distance < 0.0f))
    {
        LOG("COLLISION!");
        std::cout << x1_distance << '\n';
        g_ball_movement.x *= -1.0f;
        g_ball_movement.y *= -1.0f;

        g_player1_paddle_next = !g_player1_paddle_next;  // change which paddle hits the ball next

        // randomly move ball y direction
        g_ball_movement.y *= (rand() % 2) ? 1.0f : -1.0f;
    }

    // Move paddle based on key presses
    g_player1_matrix = glm::translate(g_player1_matrix, g_player1_position);
    g_player2_matrix = glm::translate(g_player2_matrix, g_player2_position);

    // Consume battery charge

    // Move bolt
    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

    // Set ball rotation
    if (g_ball_movement.x > 0.0f)
    {
        g_ball_rotation = 90.0f;
    }
    else
    {
        g_ball_rotation = -90.0f;
    }
    if ((g_ball_movement.y > 0.0f && g_ball_movement.x < 0.0f) || (g_ball_movement.y < 0.0f && g_ball_movement.x > 0.0f))
    {
		g_ball_rotation -= 45.0f;
    }
    else {
        g_ball_rotation += 45.0f;
    }

    // g_ball_matrix       = glm::rotate(g_ball_matrix, glm::radians(g_ball_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    g_ball_matrix = glm::rotate(g_ball_matrix, glm::radians(g_ball_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    // g_obj2_model_matrix = glm::rotate(g_obj2_model_matrix, glm::radians(g_obj2_rotation), glm::vec3(0.0f, 0.0f, 1.0f));

    // Check if ball is out of bounds
    if (g_ball_position.y > 3.5f || g_ball_position.y < -3.5f)
    {
        g_ball_movement.y *= -1.0f;
    }

    if (g_ball_position.x > 5.0f || g_ball_position.x < -5.0f)
    {
        // game over
        g_game_is_running = false;
        // // Reset ball position
        // g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
        // g_ball_matrix = glm::mat4(1.0f);
        // g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

        // // Reset ball movement
        // g_ball_movement = glm::vec3(-1.0f, 0.0f, 0.0f);
    }

    // �������������������������������������������������������������������� //
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f  // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_player1_matrix, g_player1_texture_id);
    draw_object(g_player2_matrix, g_player2_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char *argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
