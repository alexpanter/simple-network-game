
//
// Window Library
//
// Create and manage OpenGL (GLFW) drawing windows.
//

#pragma once

// GLEW
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>

// STANDARD LIBRARIES
#include <iostream>

// valid openGL function signature for key callback functions
typedef void (*_key_callback_func)        (GLFWwindow* win,int key,int scancode,
                                                           int action,int mode);
typedef void (*_framebuffer_size_callback)(GLFWwindow* win,int width,int height);
typedef void (*_mouse_callback_func)      (GLFWwindow* win,double xpos,double ypos);
typedef void (*_scroll_callback_func)     (GLFWwindow* win,double xoffset,double yoffset);

namespace Windows
{
    // --- ASPECT RATIO --- //

    // define width/height aspect ratio for a window
    typedef enum {
        ASPECT_RATIO_16_9,  // HD video
        ASPECT_RATIO_4_3,   // standard monitor
        ASPECT_RATIO_3_2,   // classic film
        ASPECT_RATIO_21_9,  // cinemascope
        ASPECT_RATIO_1_1    // quadratic window
    } as_ratio_t;

    int get_aspect_ratio_height(int width, as_ratio_t aspect)
    {
        int height;

        switch(aspect) {
        case ASPECT_RATIO_16_9:
            height = width / 16 * 9;
            break;
        case ASPECT_RATIO_21_9:
            height = width / 21 * 9;
            break;
        case ASPECT_RATIO_3_2:
            height = width / 3 * 2;
            break;
        case ASPECT_RATIO_4_3:
            height = width / 4 * 3;
            break;
        case ASPECT_RATIO_1_1:
            height = width;
            break;
        default:
            std::cout << "Error - undefined aspect ratio!" << std::endl;
            height = 0;
        }

        return height;
    }


    // --- INITIALIZING FUNCTIONS --- //

    void init_GLFW(void)
    {
        if(glfwInit() == GL_FALSE)
        {
            std::cout << "Could not initialize GLFW!" << std::endl;
        }
    }

    void init_GLEW(void)
    {
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            std::cout << "Failed to initialize GLEW" << std::endl;
        }
    }

    void set_window_hints_default(void)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    void set_window_hints_windowed(void)
    {
        set_window_hints_default();
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    }

    void set_window_hints_fullscreen(void)
    {
        set_window_hints_default();
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    }


    // --- WRAPPER CLASSES --- //

    class BaseWindow
    {
    protected:
        std::string _title;
        int _width;
        int _height;

        GLFWwindow* _window;
        as_ratio_t _as_ratio;

        bool _is_minimized = false;

        bool _wireframe = 0;

        unsigned long _clear_bits = GL_COLOR_BUFFER_BIT;

    public:
        // universal destructor, de-allocating the window resources
        virtual ~BaseWindow()
        {
            glfwDestroyWindow(_window);
            glfwTerminate();
        }

        // accessing the window
        GLFWwindow* GetWindow()
        {
            return _window;
        }

        std::string GetTitle()
        {
            return _title;
        }

        void SetTitle(std::string new_title)
        {
            _title = new_title;
            glfwSetWindowTitle(_window, _title.c_str());
        }

        int GetWidth()
        {
            return _width;
        }

        int GetHeight()
        {
            return _height;
        }

        bool IsRunning()
        {
            return !glfwWindowShouldClose(_window);
        }

        // this function does not actually close the window,
        // but marks it as 'ready to close'
        void CloseWindow()
        {
            glfwSetWindowShouldClose(_window, GL_TRUE);
        }


        // --- RENDERING METHODS --- //

        // Double buffering, should be done at the end of each
        // iteration of the rendering loop
        void SwapBuffers()
        {
            glfwSwapBuffers(_window);
        }

        // clear the window to prevent artifacts from the previous
        // iteration of the rendering loop
        void ClearWindow()
        {
            glClear(_clear_bits);
        }

        void SetClearColor(GLfloat r, GLfloat g, GLfloat b)
        {
            glClearColor(r, g, b, 1.0f);
        }
        void SetClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
        {
            glClearColor(r, g, b, a);
        }

        void SetClearBits(unsigned long bits)
        {
            _clear_bits = bits;
        }

        void SwitchWireframeMode()
        {
            if(_wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            _wireframe = !_wireframe;
        }


        // --- EVENT HANDLING --- //

        // this method adds any incoming events to openGL's
        // event queue. Should be called at the start of every
        // iteration of the game loop
        void PollEvents()
        {
            glfwPollEvents();
        }

        // this method does the same as the above, except
        // here the main thread is blocked until any event is received
        void WaitEvents()
        {
            glfwWaitEvents();
        }

        // use the provided function as primary key callback function
        // the function *must* not be instantiated.
        void SetKeyCallback(_key_callback_func func)
        {
            glfwSetKeyCallback(_window, func);
        }



        // WARNING - DO NOT USE:
        #ifdef WINDOW_UNSAFE
        // this function may only be called from the main thread
        void ChangeAspectRatio(as_ratio_t new_as_ratio)
        {
            _height = get_aspect_ratio_height(_width, new_as_ratio);
            glfwSetWindowSize(_window, _width, _height);
        }
        #endif
    }; // class BaseWindow


    class WindowedWindow : public BaseWindow
    {
    private:
    public:
        WindowedWindow(const char* title, int width, as_ratio_t aspect)
        {
            init_GLFW();
            set_window_hints_windowed();

            // height according to specified aspect ratio
            _width = width;
            _height = get_aspect_ratio_height(_width, aspect);

            // parameters: (height, width, title, monitor, share)
            _window = glfwCreateWindow(_width, _height, title, NULL, NULL);

            // check if the window could be created
            if(_window == NULL)
            {
                std::cerr << "Failed to create GLFW windowed window" << std::endl;
                glfwTerminate();
                return;
            }

            // activating the window within the current thread,
            // should be done from the main thread
            glfwMakeContextCurrent(_window);

            // tell OpenGL the size of the rendering window
            // this information is retrieved from GLFW
            glfwGetFramebufferSize(_window, &_width, &_height);

            // parameters: bottom-left corner and the dimensions of the rendering window
            glViewport(0, 0, _width, _height);
            _title = title;

            init_GLEW();
        }

        ~WindowedWindow() {}
    };


    // THIS CLASS NEEDS SOME CHECK !!
    //
    // Is it necessary to provide an aspect ratio for a fullscreen
    // window?
    //
    class FullscreenWindow : public BaseWindow
    {
    private:
        GLFWmonitor* _monitor;
        const GLFWvidmode* _mode;

    public:
        FullscreenWindow(const char* title, as_ratio_t /*aspect*/)
        {
            init_GLFW();
            set_window_hints_fullscreen();

            _monitor = glfwGetPrimaryMonitor();
            _mode = glfwGetVideoMode(_monitor);

            // height according to specified aspect ratio
            _height = get_aspect_ratio_height(_mode->width, _as_ratio);

            // parameters: (height, width, title, monitor, share)
            _window = glfwCreateWindow(_mode->width, _height, title, _monitor, NULL);

            // check if the window could be created
            if(_window == NULL) {
                std::cerr << "Failed to create GLFW fullscreen window" << std::endl;
                glfwTerminate();
                return;
            }

            // activating the window within the current thread,
            // should be done from the main thread
            glfwMakeContextCurrent(_window);

            // tell OpenGL the size of the rendering window
            // this information is retrieved from GLFW
            glfwGetFramebufferSize(_window, &_width, &_height);

            // parameters: bottom-left corner and the dimensions of the rendering window
            glViewport(0, 0, _width, _height);

            init_GLEW();
        }

        ~FullscreenWindow() {}
    };

} // namespace Window
