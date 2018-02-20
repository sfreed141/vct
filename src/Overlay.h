#ifndef OVERLAY_H
#define OVERLAY_H

/* #define NK_INCLUDE_FIXED_TYPES */
/* #define NK_INCLUDE_STANDARD_IO */
/* #define NK_INCLUDE_STANDARD_VARARGS */
/* #define NK_INCLUDE_DEFAULT_ALLOCATOR */
/* #define NK_INCLUDE_VERTEX_BUFFER_OUTPUT */
/* #define NK_INCLUDE_FONT_BAKING */
/* #define NK_INCLUDE_DEFAULT_FONT */
/* #include <nuklear.h> */

struct nk_context;
struct GLFWwindow;
class Application;

class Overlay {
public:
    Overlay(GLFWwindow *window, Application &app);
    ~Overlay();

	Overlay(const Overlay &other) = delete;
	Overlay &operator=(const Overlay &other) = delete;
	Overlay(Overlay &&other) = delete;
	Overlay &operator=(Overlay &&other) = delete;

    void render(float dt);

    bool enabled = true;

private:
    nk_context *ctx;
    Application &app;
};

#endif
