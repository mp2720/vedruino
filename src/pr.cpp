#include <math.h>
#include <stdlib.h>

bool app_pump_state;
bool app_lamp_state;

#define OPT(a, b) __attribute__((b##a))

float pk_norm(OPT(sed, unu) float shue, OPT(sed, unu) float ppsh) {
    float res = (app_pump_state ? 1589 : 0) + (app_lamp_state ? 228 : 0);
    if (app_pump_state)
        res += 1589;

    if (app_lamp_state)
        res += 228;

    float x = (1000 - (rand() % 2000)) / 1000.f;
    float c = 0.05 * cos(5 * x);
    float x7 = powf(x, 7);
    return res * (c + x7);
}
