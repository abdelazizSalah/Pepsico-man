#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>

#include <functional>
#include <array>
#include "menu-state.hpp"

#include <irrKlang.h>

// This state shows how to use some of the abstractions we created to make a menu.
class LevelsState : public our::State {

    // A meterial holding the menu shader and the menu texture to draw
    our::TexturedMaterial *menuMaterial;
    // A material to be used to highlight hovered buttons (we will use blending to create a negative effect).
    our::TintedMaterial *highlightMaterial;
    // A rectangle mesh on which the menu material will be drawn
    our::Mesh *rectangle;
    // A variable to record the time since the state is entered (it will be used for the fading effect).
    float time;
    // An array of the button that we can interact with
    std::array<Button, 3> buttons;
#ifdef USE_SOUND
    // For sound effects
    irrklang::ISoundEngine *soundEngine;
#endif
    // Used to detect button hover (for sound display)
    bool buttonHover;

    void onInitialize() override {
        buttonHover = false;

        // First, we create a material for the menu's background
        menuMaterial = new our::TexturedMaterial();
        // Here, we load the shader that will be used to draw the background
        menuMaterial->shader = new our::ShaderProgram();
        menuMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        menuMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        menuMaterial->shader->link();
        // Then we load the menu texture
        menuMaterial->texture = our::texture_utils::loadImage("assets/textures/levels.png");
        // Initially, the menu material will be black, then it will fade in
        menuMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        // Second, we create a material to highlight the hovered buttons
        highlightMaterial = new our::TintedMaterial();
        // Since the highlight is not textured, we used the tinted material shaders
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        // The tint is white since we will subtract the background color from it to create a negative effect.
        highlightMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        // To create a negative effect, we enable blending, set the equation to be subtract,
        // and set the factors to be one for both the source and the destination. 
        highlightMaterial->pipelineState.blending.enabled = true;
        highlightMaterial->pipelineState.blending.equation = GL_FUNC_SUBTRACT;
        highlightMaterial->pipelineState.blending.sourceFactor = GL_ONE;
        highlightMaterial->pipelineState.blending.destinationFactor = GL_ONE;

        // Then we create a rectangle whose top-left corner is at the origin and its size is 1x1.
        // Note that the texture coordinates at the origin is (0.0, 1.0) since we will use the
        // projection matrix to make the origin at the the top-left corner of the screen.
        rectangle = new our::Mesh({
                                          {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                                          {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                                          {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                                          {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                                  }, {
                                          0, 1, 2, 2, 3, 0,
                                  });

        // Reset the time elapsed since the state is entered.
        time = 0;

        // Fill the positions, sizes and actions for the menu buttons
        // Note that we use lambda expressions to set the actions of the buttons.
        // A lambda expression consists of 3 parts:
        // - The capture list [] which is the variables that the lambda should remember because it will use them during execution.
        //      We store [this] in the capture list since we will use it in the action.
        // - The argument list () which is the arguments that the lambda should receive when it is called.
        //      We leave it empty since button actions receive no input.
        // - The body {} which contains the code to be executed.
        buttons[0].position = {140.0f, 107.0f};
        buttons[0].size = {275.0f, 70.0f};
        buttons[0].action = [this]() {
            this->getApp()->levelState = 1;
            this->getApp()->countPepsi = 0;
            this->getApp()->heartCount = 3;
            this->getApp()->changeState("play");    // change to play state with level1
        };

        buttons[1].position = {90.0f, 300.0f};
        buttons[1].size = {380.0f, 80.0f};
        buttons[1].action = [this]() {
            this->getApp()->levelState = 2;
            this->getApp()->countPepsi = 0;
            this->getApp()->heartCount = 2;
            this->getApp()->changeState("play"); // change to play state with level2
        };

        buttons[2].position = {140.0f, 525.0f};
        buttons[2].size = {275.0f, 70.0f};
        buttons[2].action = [this]() {
            this->getApp()->levelState = 3; 
            this->getApp()->countPepsi = 0;
            this->getApp()->heartCount = 1; 
            this->getApp()->changeState("play"); // change to play state  with level3
        };
#ifdef USE_SOUND
        // Plat state sound
        soundEngine = irrklang::createIrrKlangDevice();
        soundEngine->play2D("audio/levelsState.mp3", true);
#endif
    }

    void onDraw(double deltaTime) override {
        // Get a reference to the keyboard object
        auto &keyboard = getApp()->getKeyboard();

        if (keyboard.justPressed(GLFW_KEY_SPACE)) {
            // If the space key is pressed in this frame, go to the play state with level1
            getApp()->levelState = 1; 
            getApp()->countPepsi = 0;
            getApp()->heartCount = 3; 
            getApp()->changeState("play");
        } else if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            // If the escape key is pressed in this frame, got to menu 
            getApp()->changeState("menu");
        }

        // Get a reference to the mouse object and get the current mouse position
        auto &mouse = getApp()->getMouse();
        glm::vec2 mousePosition = mouse.getMousePosition();

        // If the mouse left-button is just pressed, check if the mouse was inside
        // any menu button. If it was inside a menu button, run the action of the button.
        if (mouse.justPressed(0)) {
            for (auto &button: buttons) {
                if (button.isInside(mousePosition))
                    button.action();
            }
        }

        // Get the framebuffer size to set the viewport and the create the projection matrix.
        glm::ivec2 size = getApp()->getFrameBufferSize();
        // Make sure the viewport covers the whole size of the framebuffer.
        glViewport(0, 0, size.x, size.y);

        // The view matrix is an identity (there is no camera that moves around).
        // The projection matrix applys an orthographic projection whose size is the framebuffer size in pixels
        // so that the we can define our object locations and sizes in pixels.
        // Note that the top is at 0.0 and the bottom is at the framebuffer height. This allows us to consider the top-left
        // corner of the window to be the origin which makes dealing with the mouse input easier.
        glm::mat4 VP = glm::ortho(0.0f, (float) size.x, (float) size.y, 0.0f, 1.0f, -1.0f);
        // The local to world (model) matrix of the background which is just a scaling matrix to make the menu cover the whole
        // window. Note that we defind the scale in pixels.
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        // First, we apply the fading effect.
        time += (float) deltaTime;
        menuMaterial->tint = glm::vec4(glm::smoothstep(0.00f, 2.00f, time));
        // Then we render the menu background
        // Notice that I don't clear the screen first, since I assume that the menu rectangle will draw over the whole
        // window anyway.
        menuMaterial->setup();
        menuMaterial->shader->set("transform", VP * M);
        rectangle->draw();

        bool isHover = false;
        // For every button, check if the mouse is inside it. If the mouse is inside, we draw the highlight rectangle over it.
        for (auto &button: buttons) {
            if (button.isInside(mousePosition)) {
                if (!buttonHover) {
                    buttonHover = true;
#ifdef USE_SOUND
                    soundEngine->play2D("audio/button.mp3");
#endif
                }
                isHover = true;
                highlightMaterial->setup();
                highlightMaterial->shader->set("transform", VP * button.getLocalToWorld());
                rectangle->draw();
            }
        }
        if (!isHover)
            buttonHover = false;


    }

    void onDestroy() override {
#ifdef USE_SOUND
        // Drop sound engine
        soundEngine->drop();
#endif
        // Delete all the allocated resources
        delete rectangle;
        delete menuMaterial->texture;
        delete menuMaterial->shader;
        delete menuMaterial;
        delete highlightMaterial->shader;
        delete highlightMaterial;
    }
};