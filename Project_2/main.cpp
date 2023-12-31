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
 *   - running out of charge slows the battery down
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

#include "ShaderProgram.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"
#include "stb_image.h"

const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL  = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER   = 0;  // this value MUST be zero

const char BACKGROUND_FILEPATH[]         = "assets/background.png";
const char PLAYER_SPRITE_100_FILEPATH[]  = "assets/battery.png";
const char PLAYER_SPRITE_50_FILEPATH[]   = "assets/battery_dark_50.png";
const char PLAYER_SPRITE_0_FILEPATH[]    = "assets/battery_dark_0.png";
const char PLAYER_SPRITE_GLOW_FILEPATH[] = "assets/battery_glow.png";
const char BALL_SPRITE_FILEPATH[]        = "assets/bolt3.png";
const char PLAYER1_WIN_FILEPATH[]        = "assets/player1_win.png";
const char PLAYER2_WIN_FILEPATH[]        = "assets/player2_win.png";

SDL_Window *g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;

// Textures
GLuint g_player1_texture_id,
    g_player2_texture_id,
    g_player_texture_id,
    g_battery_100_texture_id,
    g_battery_50_texture_id,
    g_battery_0_texture_id,
    g_battery_glow_texture_id,
    g_ball_texture_id,
    g_player1_win_texture_id,
    g_player2_win_texture_id,
    g_background_texture_id;
glm::mat4 g_view_matrix,
    g_player1_matrix,
    g_player2_matrix,
    g_ball_matrix,
    g_player1_win_matrix,
    g_player2_win_matrix,
    g_background_matrix,
    g_projection_matrix;

float g_previous_ticks = 0.0f;

// Paddle positions on edge of screen
glm::vec3 PLAYER1_INIT_POSITION = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 PLAYER2_INIT_POSITION = glm::vec3(4.5f, 0.0f, 0.0f);

// Paddle 1
glm::vec3 g_player1_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player1_movement = glm::vec3(0.0f, 0.0f, 0.0f);
float g_player1_rotation     = 0.0f;
// glm::vec3 g_player1_scale    = glm::vec3(0.0f, 0.0f, 0.0f);

// Paddle 2
glm::vec3 g_player2_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player2_movement = glm::vec3(0.0f, 0.0f, 0.0f);
float g_player2_rotation     = 0.0f;
// glm::vec3 g_player2_scale    = glm::vec3(0.0f, 0.0f, 0.0f);

// Ball
glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
float g_ball_rotation     = 0.0f;
// glm::vec3 g_ball_scale    = glm::vec3(0.0f, 0.0f, 0.0f);

bool g_player1_paddle_next;  // which paddle should hit the ball next

// How fast to move the paddles
float g_player_speed = 5.0f;  // move 1 unit per second
float g_ball_speed   = 2.5f;

// Charge level for paddles
const glm::uint MAX_CHARGE = 100;
glm::uint g_player1_charge = MAX_CHARGE;
glm::uint g_player2_charge = MAX_CHARGE;

// ai
bool g_player2_is_ai            = false;
glm::vec3 g_player2_ai_movement = glm::vec3(0.0f, 1.0f, 0.0f);  // default move up

// who won
bool g_player1_win = false;
bool g_player2_win = false;

GLuint load_texture(const char *filepath) {
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char *image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL) {
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

void initialise() {
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

    // Init textures
    g_battery_100_texture_id  = load_texture(PLAYER_SPRITE_100_FILEPATH);
    g_battery_50_texture_id   = load_texture(PLAYER_SPRITE_50_FILEPATH);
    g_battery_0_texture_id    = load_texture(PLAYER_SPRITE_0_FILEPATH);
    g_battery_glow_texture_id = load_texture(PLAYER_SPRITE_GLOW_FILEPATH);
    g_player1_texture_id      = g_battery_100_texture_id;
    g_player2_texture_id      = g_battery_100_texture_id;

    g_ball_texture_id        = load_texture(BALL_SPRITE_FILEPATH);
    g_player1_win_texture_id = load_texture(PLAYER1_WIN_FILEPATH);
    g_player2_win_texture_id = load_texture(PLAYER2_WIN_FILEPATH);
    g_background_texture_id  = load_texture(BACKGROUND_FILEPATH);

    // Init matrices
    g_player1_matrix     = glm::mat4(1.0f);
    g_player2_matrix     = glm::mat4(1.0f);
    g_background_matrix  = glm::mat4(1.0f);
    g_ball_matrix        = glm::mat4(1.0f);
    g_player1_win_matrix = glm::mat4(1.0f);
    g_player2_win_matrix = glm::mat4(1.0f);

    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    // Set initial ball movement randomly
    srand(time(NULL));  // random seed based on time so each game is different
    float x, y;
    // random x movement
    g_player1_paddle_next = (rand() % 2) ? true : false;
    if (g_player1_paddle_next) {
        x = -1.0f;
    } else {
        x = 1.0f;
    }
    // random y movement
    y                  = (rand() % 2) ? 1.0f : -1.0f;
    g_ball_movement    = glm::vec3(x, y, 0.0f);
    g_ball_movement.y *= (rand() % 2) ? 1.0f : -1.0f;

    // Scale win images
    g_player1_win_matrix = glm::scale(g_player1_win_matrix, glm::vec3(5.0f, 3.0f, 1.0f));
    g_player2_win_matrix = glm::scale(g_player2_win_matrix, glm::vec3(5.0f, 3.0f, 1.0f));

    // Scale background image
    g_background_matrix = glm::scale(g_background_matrix, glm::vec3(10.0f, 8.5f, 0.0f));

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    // Do not move if no key is pressed
    g_player1_movement = glm::vec3(0.0f);
    g_player2_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:  // End game
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    // Player 1 (w and s keyboard keys)
                    case SDLK_w:
                        g_player1_movement.y = -1.0f;
                        break;
                    case SDLK_s:
                        g_player1_movement.y = 1.0f;
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

    // Key hold for smoother movement
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    // player 1
    if (key_state[SDL_SCANCODE_W]) {
        g_player1_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_S]) {
        g_player1_movement.y = -1.0f;
    }

    // player 2
    if (key_state[SDL_SCANCODE_UP]) {
        g_player2_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_DOWN]) {
        g_player2_movement.y = -1.0f;
    }

    // Normalize movement
    if (glm::length(g_player1_movement) > 1.0f) {
        g_player1_movement = glm::normalize(g_player1_movement);
    }
    if (glm::length(g_player2_movement) > 1.0f) {
        g_player2_movement = glm::normalize(g_player2_movement);
    }
}

void update() {
    float ticks      = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;  // get the current number of ticks
    float delta_time = ticks - g_previous_ticks;                        // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    if (g_player1_win || g_player2_win) {  // Only update position if no one has won yet
        g_player1_movement    = glm::vec3(0.0f, 0.0f, 0.0f);
        g_player2_movement    = glm::vec3(0.0f, 0.0f, 0.0f);
        g_player2_ai_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }  // don't return yet so paddle position is still correct

    // Modify speed based on charge
    if (g_player1_charge > 150) {
        g_player1_movement *= 1.50f;  // bonus speed of 50%
    } else if (g_player1_charge <= 20) {
        g_player1_movement *= 0.25f;  // penalty of 75%
    }
    if (g_player2_charge > 150) {
        g_player2_movement *= 1.50f;
    } else if (g_player2_charge <= 20) {
        g_player2_movement *= 0.25f;
    }

    // Add direction * units per second * elapsed time
    g_player1_position += g_player1_movement * g_player_speed * delta_time;
    g_ball_position    += g_ball_movement * g_ball_speed * delta_time;

    // Player 2
    if (g_player2_is_ai) {  // Use AI
        // Move up and down
        if (g_player2_position.y >= 3.0f) {
            g_player2_ai_movement.y = -1.0f;
        } else if (g_player2_position.y <= -3.0f) {
            g_player2_ai_movement.y = 1.0f;
        }
        g_player2_position += g_player2_ai_movement * g_player_speed * delta_time;
    } else {  // Use key input
        g_player2_position += g_player2_movement * g_player_speed * delta_time;
    }

    // Bound paddle to top and bottom screen edges
    if (g_player1_position.y > 3.0f) {
        g_player1_position.y = 3.0f;
    } else if (g_player1_position.y < -3.0f) {
        g_player1_position.y = -3.0f;
    }
    if (g_player2_position.y > 3.0f) {
        g_player2_position.y = 3.0f;
    } else if (g_player2_position.y < -3.0f) {
        g_player2_position.y = -3.0f;
    }

    // Reset model matrix
    g_player1_matrix = glm::mat4(1.0f);
    g_player2_matrix = glm::mat4(1.0f);

    // Set paddle locations to horizontal edge of screen
    g_player1_matrix = glm::translate(g_player1_matrix, PLAYER1_INIT_POSITION);
    g_player2_matrix = glm::translate(g_player2_matrix, PLAYER2_INIT_POSITION);

    // Collision Detection
    float collision_factor = 0.40f;
    float x1_distance      = fabs(g_player1_position.x + PLAYER1_INIT_POSITION.x - g_ball_position.x) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float y1_distance      = fabs(g_player1_position.y + PLAYER1_INIT_POSITION.y - g_ball_position.y) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float x2_distance      = fabs(g_player2_position.x + PLAYER2_INIT_POSITION.x - g_ball_position.x) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float y2_distance      = fabs(g_player2_position.y + PLAYER2_INIT_POSITION.y - g_ball_position.y) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);

    // Collision with paddle
    if ((g_player1_paddle_next && x1_distance < 0.0f && y1_distance < 0.0f) || (!g_player1_paddle_next && x2_distance < 0.0f && y2_distance < 0.0f)) {
        g_ball_movement.x *= -1.0f;

        // Randomly change ball y direction
        g_ball_movement.y *= (rand() % 2) ? 1.0f : -1.0f;

        // Restore charge to paddle
        if (g_player1_paddle_next) {
            g_player1_charge += MAX_CHARGE;
        } else {
            g_player2_charge += MAX_CHARGE;
        }

        g_player1_paddle_next = !g_player1_paddle_next;  // change which paddle hits the ball next
    }

    g_player1_matrix = glm::translate(g_player1_matrix, g_player1_position);
    g_player2_matrix = glm::translate(g_player2_matrix, g_player2_position);

    // Consume battery charge
    if (g_player1_movement.x != 0.0f || g_player1_movement.y != 0.0f) {
        std::cout << "Player 1 charge: " << g_player1_charge << std::endl;
        g_player1_charge = glm::max(g_player1_charge - fabs(g_player1_movement.y), 0.0f);
    }
    if (g_player2_movement.x != 0.0f || g_player2_movement.y != 0.0f) {
        std::cout << "Player 2 charge: " << g_player2_charge << std::endl;
        g_player2_charge = glm::max(g_player2_charge - fabs(g_player2_movement.y), 0.0f);
    }

    // Update paddle texture
    if (g_player1_charge > 150) {
        g_player1_texture_id = g_battery_glow_texture_id;
    } else if (g_player1_charge > 75) {
        g_player1_texture_id = g_battery_100_texture_id;
    } else if (g_player1_charge > 20) {
        g_player1_texture_id = g_battery_50_texture_id;
    } else {
        g_player1_texture_id = g_battery_0_texture_id;
    }

    if (g_player2_is_ai) {  // AI always uses standard speed and does not consume charge
        g_player2_texture_id = g_battery_100_texture_id;
    }

    else {
        if (g_player2_charge > 150) {
            g_player2_texture_id = g_battery_glow_texture_id;
        } else if (g_player2_charge > 75) {
            g_player2_texture_id = g_battery_100_texture_id;
        } else if (g_player2_charge > 20) {
            g_player2_texture_id = g_battery_50_texture_id;
        } else {
            g_player2_texture_id = g_battery_0_texture_id;
        }
    }

    // Move bolt
    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

    // Set ball rotation
    if (g_ball_movement.x > 0.0f) {
        g_ball_rotation = 90.0f;
    } else {
        g_ball_rotation = -90.0f;
    }
    if ((g_ball_movement.y > 0.0f && g_ball_movement.x < 0.0f) || (g_ball_movement.y < 0.0f && g_ball_movement.x > 0.0f)) {
        g_ball_rotation -= 45.0f;
    } else {
        g_ball_rotation += 45.0f;
    }

    g_ball_matrix = glm::rotate(g_ball_matrix, glm::radians(g_ball_rotation), glm::vec3(0.0f, 0.0f, 1.0f));

    // Check if ball is out of bounds and set winner
    if (g_ball_position.y > 3.5f || g_ball_position.y < -3.5f) {
        g_ball_movement.y *= -1.0f;
    }

    if (g_ball_position.x > 5.0f) {
        g_player1_win = true;
    }
    if (g_ball_position.x < -5.0f) {
        g_player2_win = true;
    }
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id) {
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);  // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,  // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_background_matrix, g_background_texture_id);
    draw_object(g_player1_matrix, g_player1_texture_id);
    draw_object(g_player2_matrix, g_player2_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);
    if (g_player1_win) {
        draw_object(g_player1_win_matrix, g_player1_win_texture_id);
    }
    if (g_player2_win) {
        draw_object(g_player2_win_matrix, g_player2_win_texture_id);
    }

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char *argv[]) {
    initialise();

    while (g_game_is_running) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
