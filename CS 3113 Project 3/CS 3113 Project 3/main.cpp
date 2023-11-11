/**
* Author: Jonathan Kim
* Assignment: Lunar Lander
* Date due: 2023-11-08, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ACC_OF_GRAVITY -0.15f
#define PLATFORM_COUNT 7

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.hpp"

using namespace std;

struct GameState
{
    Entity* player;
    Entity* platforms;
    Entity* fuel;
};

const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const int FONTBANK_SIZE = 16;

const char SPRITESHEET_FILEPATH[] = "assets/george_0.png";
const char PLATFORM_FILEPATH[]    = "assets/platformPack_tile027.png";
const char TEXT_SPRITE_FILEPATH[] = "assets/font1.png";
const char FUEL_SPRITE_FILEPATH[] = "assets/fuel.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix, g_text_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

bool mission;

GLuint text_texture_id;

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("LUNAR LANDER",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.SetProjectionMatrix(g_projection_matrix);
    g_program.SetViewMatrix(g_view_matrix);
    
    g_text_matrix = glm::mat4(1.0f);
    g_text_matrix = glm::translate(g_text_matrix, glm::vec3(-3.5f, 0.0f, 0.0f));
    
    glUseProgram(g_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    
    g_state.platforms = new Entity[PLATFORM_COUNT];
    
    g_state.platforms[PLATFORM_COUNT - 1].m_texture_id = platform_texture_id;
    g_state.platforms[PLATFORM_COUNT - 1].set_position(glm::vec3(-1.5f, 2.35f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 1].m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    g_state.platforms[PLATFORM_COUNT - 1].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 1].update(0.0f, NULL, 0);
    
    for (int i = 0; i < PLATFORM_COUNT - 2; i++)
    {
        g_state.platforms[i].m_texture_id = platform_texture_id;
        g_state.platforms[i].set_position(glm::vec3(i - 1.0f, -3.0f, 0.0f));
        g_state.platforms[i].m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
        g_state.platforms[i].set_width(0.4f);
        g_state.platforms[i].update(0.0f, NULL, 0);
    }
    
    g_state.platforms[PLATFORM_COUNT - 2].m_texture_id = platform_texture_id;
    g_state.platforms[PLATFORM_COUNT - 2].set_position(glm::vec3(2.5f, -2.5f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 2].m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    g_state.platforms[PLATFORM_COUNT - 2].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 2].update(0.0f, NULL, 0);
    
    g_state.platforms[PLATFORM_COUNT - 3].m_texture_id = platform_texture_id;
    g_state.platforms[PLATFORM_COUNT - 3].set_position(glm::vec3(2.5f, 2.5f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 3].m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    g_state.platforms[PLATFORM_COUNT - 3].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 3].update(0.0f, NULL, 0);
    
    g_state.platforms[PLATFORM_COUNT - 4].m_texture_id = platform_texture_id;
    g_state.platforms[PLATFORM_COUNT - 4].set_position(glm::vec3(0.0f, 2.0f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 4].m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    g_state.platforms[PLATFORM_COUNT - 4].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 4].update(0.0f, NULL, 0);
    
    g_state.platforms[rand() % PLATFORM_COUNT].set_entity_type(TRAP);
    
    g_state.player = new Entity();
    g_state.player->set_position(glm::vec3(-4.0f, 3.0f, 0.0f));
    g_state.player->m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->m_speed = 1.0f;
    g_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY, 0.0f));
    g_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
    g_state.player->m_moving[g_state.player->LEFT]  = new int[4] { 1, 5, 9,  13 };
    g_state.player->m_moving[g_state.player->RIGHT] = new int[4] { 3, 7, 11, 15 };
    g_state.player->m_moving[g_state.player->UP]    = new int[4] { 2, 6, 10, 14 };
    g_state.player->m_moving[g_state.player->DOWN]  = new int[4] { 0, 4, 8,  12 };

    g_state.player->m_animation_indices = g_state.player->m_moving[g_state.player->DOWN];
    
    g_state.player->m_animation_frames = 4;
    g_state.player->m_animation_index  = 0;
    g_state.player->m_animation_time   = 0.0f;
    g_state.player->m_animation_cols   = 4;
    g_state.player->m_animation_rows   = 4;
    g_state.player->set_height(0.9f);
    g_state.player->set_width(0.9f);
    g_state.player->set_entity_type(PLAYER);
    
    g_state.fuel = new Entity();
    g_state.fuel->set_position(glm::vec3(-5.0f, 3.0f, 0.0f));
    g_state.fuel->set_movement(glm::vec3(0.0f));
    g_state.fuel->m_texture_id = load_texture(FUEL_SPRITE_FILEPATH);
    g_state.fuel->m_scale = glm::vec3(21.0f, 0.3f, 1.0f);
    
    
    text_texture_id = load_texture(TEXT_SPRITE_FILEPATH);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY, 0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        g_game_is_running = false;
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->set_acceleration(glm::vec3(-0.5f, ACC_OF_GRAVITY, 0.0f));
        g_state.player->m_animation_indices = g_state.player->m_moving[g_state.player->LEFT];
        if(g_state.fuel->game_over == false) {
            g_state.fuel->m_scale.x -= 0.01f;
        }
    }
    if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->set_acceleration(glm::vec3(0.5f, ACC_OF_GRAVITY, 0.0f));
        g_state.player->m_animation_indices = g_state.player->m_moving[g_state.player->RIGHT];
        if(g_state.fuel->game_over == false) {
            g_state.fuel->m_scale.x -= 0.01f;
        }
    }
    if (key_state[SDL_SCANCODE_UP])
    {
        g_state.player->set_acceleration(glm::vec3(0.0f, 0.5f + ACC_OF_GRAVITY, 0.0f));
        g_state.player->m_animation_indices = g_state.player->m_moving[g_state.player->UP];
        if(g_state.fuel->game_over == false) {
            g_state.fuel->m_scale.x -= 0.01f;
        }
    }
    if (key_state[SDL_SCANCODE_DOWN])
    {
        g_state.player->set_acceleration(glm::vec3(0.0f, -0.5f + ACC_OF_GRAVITY, 0.0f));
        g_state.player->m_animation_indices = g_state.player->m_moving[g_state.player->DOWN];
        if(g_state.fuel->game_over == false) {
            g_state.fuel->m_scale.x -= 0.001f;
        }
    }
    
    if (glm::length(g_state.player->m_movement) > 1.0f)
    {
        g_state.player->m_movement = glm::normalize(g_state.player->m_movement);
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, g_state.platforms, PLATFORM_COUNT);
        g_state.fuel->update(FIXED_TIMESTEP, NULL, 0);
        if(g_state.fuel->game_over) {
            g_state.player->game_over = g_state.fuel->game_over;
        }
        else if(g_state.player->game_over) {
            g_state.fuel->game_over = g_state.player->game_over;
        }
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
}

void DrawText(ShaderProgram *program, GLuint font_texture_id, string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    vector<float> vertices;
    vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    g_text_matrix = glm::translate(g_text_matrix, position);
    
    program->SetModelMatrix(g_text_matrix);
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_state.fuel->render(&g_program);
    
    g_state.player->render(&g_program);
    
    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
    if (g_state.player->game_over == true) {
        if (g_state.player->mission == true)
            DrawText(&g_program, text_texture_id, "MISSION SUCCESS!", 0.5f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
        else {
            DrawText(&g_program, text_texture_id, "MISSION FAILED!", 0.5f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
        }
    }
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    srand(time_t(NULL));
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}

