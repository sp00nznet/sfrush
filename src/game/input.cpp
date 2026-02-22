/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * Input handling
 *
 * Maps modern input (keyboard, gamepad) to N64 controller input.
 * SF Rush uses:
 *   - Analog stick: Steering
 *   - A button: Accelerate
 *   - B button: Brake/Reverse
 *   - Z trigger: Abort race
 *   - C buttons: Music/Horn
 *   - R trigger: View change
 *   - D-Pad: Menu navigation
 *   - Start: Pause
 */

#include <cstdio>
#include <cstdint>
#include <cstring>

#include <SDL.h>

// N64 controller button masks
#define N64_BTN_A       0x8000
#define N64_BTN_B       0x4000
#define N64_BTN_Z       0x2000
#define N64_BTN_START   0x1000
#define N64_BTN_DU      0x0800
#define N64_BTN_DD      0x0400
#define N64_BTN_DL      0x0200
#define N64_BTN_DR      0x0100
#define N64_BTN_L       0x0020
#define N64_BTN_R       0x0010
#define N64_BTN_CU      0x0008
#define N64_BTN_CD      0x0004
#define N64_BTN_CL      0x0002
#define N64_BTN_CR      0x0001

struct N64Input {
    uint16_t buttons;
    int8_t stick_x;
    int8_t stick_y;
};

static N64Input g_input = {0, 0, 0};
static SDL_GameController* g_gamepad = nullptr;

namespace sfrush {

void input_init() {
    fprintf(stderr, "[INPUT] Initializing input system\n");

    // Open first available gamepad
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            g_gamepad = SDL_GameControllerOpen(i);
            if (g_gamepad) {
                fprintf(stderr, "[INPUT] Gamepad connected: %s\n",
                        SDL_GameControllerName(g_gamepad));
                break;
            }
        }
    }

    if (!g_gamepad) {
        fprintf(stderr, "[INPUT] No gamepad found, using keyboard\n");
    }

    fprintf(stderr, "[INPUT] Keyboard mapping:\n");
    fprintf(stderr, "[INPUT]   WASD / Arrows  = Steering + Accel/Brake\n");
    fprintf(stderr, "[INPUT]   Space          = A (Accelerate)\n");
    fprintf(stderr, "[INPUT]   Left Shift     = B (Brake)\n");
    fprintf(stderr, "[INPUT]   Z              = Z Trigger\n");
    fprintf(stderr, "[INPUT]   Return         = Start\n");
    fprintf(stderr, "[INPUT]   Tab            = R (View Change)\n");
    fprintf(stderr, "[INPUT]   I/J/K/L        = C Buttons\n");
}

void input_update() {
    memset(&g_input, 0, sizeof(g_input));

    const uint8_t* keys = SDL_GetKeyboardState(nullptr);

    // Keyboard: buttons
    if (keys[SDL_SCANCODE_SPACE])   g_input.buttons |= N64_BTN_A;
    if (keys[SDL_SCANCODE_LSHIFT])  g_input.buttons |= N64_BTN_B;
    if (keys[SDL_SCANCODE_Z])       g_input.buttons |= N64_BTN_Z;
    if (keys[SDL_SCANCODE_RETURN])  g_input.buttons |= N64_BTN_START;
    if (keys[SDL_SCANCODE_TAB])     g_input.buttons |= N64_BTN_R;
    if (keys[SDL_SCANCODE_Q])       g_input.buttons |= N64_BTN_L;

    // D-Pad
    if (keys[SDL_SCANCODE_UP])      g_input.buttons |= N64_BTN_DU;
    if (keys[SDL_SCANCODE_DOWN])    g_input.buttons |= N64_BTN_DD;
    if (keys[SDL_SCANCODE_LEFT])    g_input.buttons |= N64_BTN_DL;
    if (keys[SDL_SCANCODE_RIGHT])   g_input.buttons |= N64_BTN_DR;

    // C buttons
    if (keys[SDL_SCANCODE_I])       g_input.buttons |= N64_BTN_CU;
    if (keys[SDL_SCANCODE_K])       g_input.buttons |= N64_BTN_CD;
    if (keys[SDL_SCANCODE_J])       g_input.buttons |= N64_BTN_CL;
    if (keys[SDL_SCANCODE_L])       g_input.buttons |= N64_BTN_CR;

    // Keyboard: analog stick via WASD
    int8_t sx = 0, sy = 0;
    if (keys[SDL_SCANCODE_A]) sx -= 80;
    if (keys[SDL_SCANCODE_D]) sx += 80;
    if (keys[SDL_SCANCODE_W]) sy += 80;
    if (keys[SDL_SCANCODE_S]) sy -= 80;

    // Gamepad: override with analog values if available
    if (g_gamepad) {
        int16_t gx = SDL_GameControllerGetAxis(g_gamepad, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t gy = SDL_GameControllerGetAxis(g_gamepad, SDL_CONTROLLER_AXIS_LEFTY);

        // Map 16-bit range to N64's -80..+80 range
        sx = (int8_t)(gx / 410);
        sy = (int8_t)(-gy / 410); // Y axis inverted

        // Gamepad buttons
        if (SDL_GameControllerGetButton(g_gamepad, SDL_CONTROLLER_BUTTON_A))
            g_input.buttons |= N64_BTN_A;
        if (SDL_GameControllerGetButton(g_gamepad, SDL_CONTROLLER_BUTTON_B))
            g_input.buttons |= N64_BTN_B;
        if (SDL_GameControllerGetButton(g_gamepad, SDL_CONTROLLER_BUTTON_X))
            g_input.buttons |= N64_BTN_B; // X also brake
        if (SDL_GameControllerGetButton(g_gamepad, SDL_CONTROLLER_BUTTON_START))
            g_input.buttons |= N64_BTN_START;

        // Triggers
        int16_t lt = SDL_GameControllerGetAxis(g_gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        int16_t rt = SDL_GameControllerGetAxis(g_gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (lt > 8000) g_input.buttons |= N64_BTN_Z;
        if (rt > 8000) g_input.buttons |= N64_BTN_R;

        // Right stick as C buttons
        int16_t rx = SDL_GameControllerGetAxis(g_gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
        int16_t ry = SDL_GameControllerGetAxis(g_gamepad, SDL_CONTROLLER_AXIS_RIGHTY);
        if (rx < -12000) g_input.buttons |= N64_BTN_CL;
        if (rx >  12000) g_input.buttons |= N64_BTN_CR;
        if (ry < -12000) g_input.buttons |= N64_BTN_CU;
        if (ry >  12000) g_input.buttons |= N64_BTN_CD;
    }

    g_input.stick_x = sx;
    g_input.stick_y = sy;
}

N64Input* input_get_state() {
    return &g_input;
}

void input_shutdown() {
    if (g_gamepad) {
        SDL_GameControllerClose(g_gamepad);
        g_gamepad = nullptr;
    }
    fprintf(stderr, "[INPUT] Input system shut down\n");
}

} // namespace sfrush
