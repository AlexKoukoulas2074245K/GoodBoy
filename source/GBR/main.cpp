#include <SDL.h>
#include <memory>
#include <memory.h>

#include "logging.h"
#include "system.h" 
#include "types.h"

#include <fstream>

struct SDLWindowDeleter
{
    void operator()(SDL_Window* window)
    {
        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
        }
    }
};

struct SDLRendererDeleter
{
    void operator()(SDL_Renderer* renderer)
    {
        if (renderer != nullptr)
        {
            SDL_DestroyRenderer(renderer);
        }
    }
};

struct SDLTextureDeleter
{
    void operator()(SDL_Texture* texture)
    {
        if (texture != nullptr)
        {
            SDL_DestroyTexture(texture);
        }
    }
};

void Render(byte* pixels, SDL_Renderer* pRenderer, SDL_Texture* pTexture)
{
    // Clear window
    SDL_SetRenderDrawColor(pRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(pRenderer);

    byte* pPixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(pTexture, nullptr, (void**)&pPixels, &pitch);

    // Render Game
    memcpy(pPixels, pixels, 160 * 144 * 4);

    SDL_UnlockTexture(pTexture);

    SDL_RenderCopy(pRenderer, pTexture, nullptr, nullptr);

    // Update window
    SDL_RenderPresent(pRenderer);
}

// TODO: refactor this
std::unique_ptr<SDL_Renderer, SDLRendererDeleter> spRenderer;
std::unique_ptr<SDL_Texture, SDLTextureDeleter> spTexture;

// The emulator will call this whenever we hit VBlank
void VBlankCallback(byte* pixels)
{
    Render(pixels, spRenderer.get(), spTexture.get());
}

void ProcessInput(System& system)
{
    SDL_PumpEvents();
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    byte actionButtons = 0;
    byte directionButtons = 0;

    if (keys[SDL_SCANCODE_UP])
    {
        directionButtons |= System::DIRECTION_BUTTON_UP_MASK;
    }

    if (keys[SDL_SCANCODE_LEFT])
    {
        directionButtons |= System::DIRECTION_BUTTON_LEFT_MASK;
    }

    if (keys[SDL_SCANCODE_DOWN])
    {
        directionButtons |= System::DIRECTION_BUTTON_DOWN_MASK;
    }

    if (keys[SDL_SCANCODE_RIGHT])
    {
        directionButtons |= System::DIRECTION_BUTTON_RIGHT_MASK;
    }

    if (keys[SDL_SCANCODE_Z])
    {
        actionButtons |= System::ACTION_BUTTON_A_MASK;
    }

    if (keys[SDL_SCANCODE_X])
    {
        actionButtons |= System::ACTION_BUTTON_B_MASK;
    }

    if (keys[SDL_SCANCODE_RETURN])
    {
        actionButtons |= System::ACTION_BUTTON_START_MASK;
    }

    if (keys[SDL_SCANCODE_BACKSPACE])
    {
        actionButtons |= System::ACTION_BUTTON_SELECT_MASK;
    }

    system.setInputState(actionButtons, directionButtons);
}


// 60 FPS or 16.67ms
const double TimePerFrame = 1.0 / 60.0;

// The number of CPU clock cycles per frame
constexpr unsigned int CPU_CLOCK_CYCLES_PER_FRAME = 70224;


int main(int argc, char** argv)
{
    int windowWidth = 160;
    int windowHeight = 144;
    int windowScale = 3;
    bool isRunning = true;

    std::unique_ptr<SDL_Window, SDLWindowDeleter> spWindow;
    SDL_Event event;

    /*std::ifstream gladFile("glad_out.txt");
    std::ifstream ourFile("out.txt");

    std::string line1, line2;
    std::string line1Prev, line2Prev;

    while (std::getline(gladFile, line1))
    {
        std::getline(ourFile, line2);

        if (line1.substr(17) != line2.substr(17))
        {
            log(LogType::INFO, "DIFF: \n     %s\n     %s", line1.c_str(), line2.c_str());
            log(LogType::INFO, "Prev: \n     %s\n     %s", line1Prev.c_str(), line2Prev.c_str());
        }


        line1Prev = line1;
        line2Prev = line2;
    }*/

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        log(LogType::ERROR, "SDL could not initialize! SDL error: '%s'", SDL_GetError());
        return -1;
    }

    // Create window
    spWindow = std::unique_ptr<SDL_Window, SDLWindowDeleter>(
        SDL_CreateWindow(
            "GBR",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            windowWidth * windowScale, // Original = 160
            windowHeight * windowScale, // Original = 144
            SDL_WINDOW_SHOWN));
    if (spWindow == nullptr)
    {
        log(LogType::ERROR, "Window could not be created! SDL error: '%s'", SDL_GetError());
        return false;
    }

    // Create renderer
    spRenderer = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>(
        SDL_CreateRenderer(spWindow.get(), -1, SDL_RENDERER_ACCELERATED));
    if (spRenderer == nullptr)
    {
        log(LogType::ERROR, "Renderer could not be created! SDL error: '%s'", SDL_GetError());
        return false;
    }

    spTexture = std::unique_ptr<SDL_Texture, SDLTextureDeleter>(
        SDL_CreateTexture(spRenderer.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144));

    // set vsync data
    unsigned int cpuClockCycles = 0;
    
    System dmgSystem; 
    //const auto cartridgeName = dmgSystem.loadCartridge("roms/tetris.gb");
    
    std::string cartridgeName;
    if (argc == 1)
        cartridgeName = dmgSystem.loadCartridge("roms/red.gb");
    else
        cartridgeName = dmgSystem.loadCartridge(argv[argc - 1]);

    dmgSystem.setVBlankCallback(VBlankCallback);

    SDL_SetWindowTitle(spWindow.get(), ("GBR: Emulating " + cartridgeName).c_str());

    Uint64 frameStart = SDL_GetPerformanceCounter();
    while (isRunning)
    {
        // Poll for window input
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                isRunning = false;
            }
        }

        if (!isRunning)
        {
            // Exit early if the app is closing
            continue;
        }

        ProcessInput(dmgSystem);
        while (cpuClockCycles < CPU_CLOCK_CYCLES_PER_FRAME)
        {
            cpuClockCycles += dmgSystem.emulateNextMachineStep();
        }

        cpuClockCycles -= CPU_CLOCK_CYCLES_PER_FRAME;

        Uint64 frameEnd = SDL_GetPerformanceCounter();
        // Loop until we use up the rest of our frame time
        while (true)
        {
            frameEnd = SDL_GetPerformanceCounter();
            double frameElapsedInSec = (double)(frameEnd - frameStart) / SDL_GetPerformanceFrequency();

            // Break out once we use up our time per frame
            if (frameElapsedInSec >= TimePerFrame)
            {
                break;
            }
        }

        frameStart = frameEnd;
    }

    spTexture.reset();
    spRenderer.reset();
    spWindow.reset();
    SDL_Quit();

    return 0;
}
