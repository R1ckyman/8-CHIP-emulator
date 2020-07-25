#include <SDL.h>
#include "Chip8.h"
#include <chrono>
#include <thread>
// https://github.com/ocornut/imgui
#include "imgui.h"
// https://github.com/Tyyppi77/imgui_sdl
#include "imgui_sdl.h"
// https://github.com/mlabbe/nativefiledialog
#include <nfd.h>

unsigned long createRGB(int r, int g, int b, int a = 0xFF);
void ShowInformation();
void ShowMenu();
bool SelectGame();

const std::string VERSION = "v1.0";
const int WIDTH = 1280;
const int HEIGHT = 640;

int max_fps = 60;
bool game_loaded = false;
bool sound_timer = true;
bool game_paused = false;
bool imgui_visible = true;
std::string current_game = "";

unsigned long pixel_color = 0xFFFFFFFF;
bool rainbow_mode = false;

uint8_t keymap[16] = {
    SDLK_x, // 0
    SDLK_1, // 1
    SDLK_2, // 2
    SDLK_3, // 3
    SDLK_q, // 4
    SDLK_w, // 5
    SDLK_e, // 6
    SDLK_a, // 7
    SDLK_s, // 8
    SDLK_d, // 9
    SDLK_z, // A
    SDLK_c, // B
    SDLK_4, // C
    SDLK_r, // D
    SDLK_f, // E     
    SDLK_v, // F
};

Chip8 chip8 = Chip8();

int main(int argc, char* args[]) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> random(50, 255);
    SDL_Window* window = NULL;
    SDL_Window* window_imgui = NULL;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Create window
    window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create renderer      SDL_RENDERER_SOFTWARE as a workaround for fixing the render clearing
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == nullptr)
    {
        std::cout << "Error in initializing rendering: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

    ImGui::CreateContext();
    ImGuiSDL::Initialize(renderer, WIDTH, HEIGHT);
    ImGui::SetNextWindowPos(ImVec2{0, 0});
    ImGui::SetNextWindowSize(ImVec2{ WIDTH, HEIGHT });

    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        64, 32);
    if (texture == nullptr)
    {
        std::cerr << "Error in setting up texture: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point b = std::chrono::system_clock::now();
    int draw_timer = 60;

	while (chip8.GetState() == State::ON)
	{
        ImGuiIO& io = ImGui::GetIO();

        // 60Hz
        a = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> work_time = a - b;

        if (work_time.count() < 200.0)
        {
            std::chrono::duration<double, std::milli> delta_ms((1000 / max_fps) - work_time.count());
            auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
        }

        b = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> sleep_time = b - a;
        if (game_loaded && !game_paused) {
            // Cycle
            chip8.EmulateCycle(sound_timer);
        }

        int wheel = 0;

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type)
            {
            case SDL_QUIT:
                chip8.SetState(State::OFF);
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    chip8.SetState(State::OFF);
                }
                if (event.key.keysym.sym == SDLK_F1) {
                    imgui_visible = !imgui_visible;
                }
                for (int i = 0; i < 16; i++)
                {
                    if (event.key.keysym.sym == keymap[i]) {
                        chip8.SetKey(i, true);
                    }
                }
                break;
            case SDL_KEYUP:
                for (int i = 0; i < 16; i++)
                {
                    if (event.key.keysym.sym == keymap[i]) {
                        chip8.SetKey(i, false);
                    }
                }
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_CLOSE:
                    chip8.SetState(State::OFF);
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
        if (game_paused || !game_loaded) {
            SDL_RenderClear(renderer);
        }
        if (chip8.GetDrawFlag() || draw_timer == 0) {
            chip8.SetDrawFlag(false);
            draw_timer = 30;
            uint32_t pixels[32 * 64];
            for (int i = 0; i < 32 * 64; i++)
            {
                if (chip8.GetPixel(i) == 0)
                {
                    pixels[i] = 0xFF000000;
                }
                else
                {
                    if (rainbow_mode) {
                        pixel_color = createRGB(random(mt), random(mt), random(mt));
                    }
                    pixels[i] = (0xFFFFFFFF & pixel_color);
                }
            }

            SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t));
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        }
        int mouseX, mouseY;
        const int buttons = SDL_GetMouseState(&mouseX, &mouseY);

        io.DeltaTime = 1.0f / 60.0f;
        io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
        io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
        io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
        io.MouseWheel = static_cast<float>(wheel);

        ImGui::NewFrame();

        if (imgui_visible) {
            ShowMenu();
            ShowInformation();
        }

        ImGui::Render();
        ImGuiSDL::Render(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);

        if (draw_timer > 0) {
            draw_timer--;
        }
        double frames = (work_time + sleep_time).count();
        SDL_SetWindowTitle(window, ("Chip 8 emulator | FPS: " + std::string(std::to_string(1000 / frames)) + std::string(", Frames: ") + std::string(std::to_string(frames)) + std::string(" (ms)")).c_str());
	}

    ImGuiSDL::Deinitialize();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    ImGui::DestroyContext();

	return 0;
}

void ShowInformation() {
    if (ImGui::Begin("Info")) {
        ImGui::Text("Chip 8 emulator written in c++");
        ImGui::Text("You can disable the \"Beep\" sound in settings");
        ImGui::Text("Press F1 to hide/show the menu and info!");
        ImGui::Text("Control the games with:");
        ImGui::Text("1  2  3  4");
        ImGui::Text("q  w  e  r");
        ImGui::Text("a  s  d  f");
        ImGui::Text("z  x  c  v");
    }
    ImGui::End();
}

static void ShowMenu()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Menu")) {
            if (ImGui::MenuItem("Open")) {
                if (SelectGame()) {
                    if (game_loaded) {
                        chip8.Reset();
                    }
                    game_loaded = true;
                    bool result = chip8.LoadGame(current_game);

                    if (!result)
                    {
                        game_loaded = false;
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Pause", "", false, !game_paused)) {
                game_paused = true;
            }
            if (ImGui::MenuItem("Resume", "", false, game_paused)) {
                game_paused = false;
            }
            
            if (ImGui::MenuItem("Reset", "", false, game_loaded)) {
                chip8.Reset();
                chip8.LoadGame(current_game);
            }

            if (ImGui::MenuItem("Stop", "", false, game_loaded)) {
                chip8.Reset();
                game_loaded = false;
                game_paused = false;
                current_game = "";
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                chip8.SetState(State::OFF);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::Checkbox("Enable the sound timer \"Beep\"", &sound_timer);
            if (ImGui::BeginMenu("FPS limit")) {
                const char* items[] = { "15 fps", "30 fps", "60 fps", "120 fps", "144 fps", "360 fps", "720 fps"};
                static int item_current = 2;
                ImGui::Combo("Max", &item_current, items, IM_ARRAYSIZE(items));
                switch (item_current)
                {
                case 0:
                    max_fps = 15;
                    break;
                case 1:
                    max_fps = 30;
                    break;
                case 2:
                    max_fps = 60;
                    break;
                case 3:
                    max_fps = 120;
                    break;
                case 4:
                    max_fps = 144;
                    break;
                case 5:
                    max_fps = 360;
                    break;
                case 6:
                    max_fps = 720;
                    break;
                default:
                    max_fps = 60;
                    break;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Pixel color"))
            {   
                ImGui::Checkbox("Colorful mode", &rainbow_mode);
                static ImVec4 color = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
                ImGui::Text("Pixel color picker:");
                ImGui::ColorPicker4("Pixel color", (float*)&color);

                pixel_color = createRGB(std::round(color.x * 0xFF), std::round(color.y * 0xFF), std::round(color.z * 0xFF));
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("About"))
        {
            ImGui::Text(("Version: " + VERSION).c_str());
            if (ImGui::MenuItem("Source code")) {
                //  Windows
                system("start https://github.com/R1ckyman/Chip-8-emulator");
            }
            
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

unsigned long createRGB(int r, int g, int b, int a) {
    return (a << 24) + ((r & 0xff) << 16) + ((g & 0xff) << 8) + ((b & 0xff));
}

bool SelectGame() {
    nfdchar_t* outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

    if (result == NFD_OKAY) {
        current_game = outPath;
        std::cout << "Path: " << current_game << std::endl;
        free(outPath);
        return true;
    }
    else if (result == NFD_CANCEL) {
        std::cout << "Cancelled" << std::endl;
    }
    else {
        printf("Error: %s\n", NFD_GetError());
    }

    return false;
}