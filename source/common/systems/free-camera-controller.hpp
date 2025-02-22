#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/player.hpp"
#include "../components/free-camera-controller.hpp"

#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

#ifdef USE_SOUND

#include <irrKlang.h>

#endif

namespace our {

    // The state of jumping and falling
    enum JumpState {
        JUMPING,
        FALLING,
        GROUNDED
    };
    // The state of sliding
    enum SlideState {
        Slided,
        NORMAL,
    };

    // The free camera controller system is responsible for moving every entity which contains a FreeCameraControllerComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic.
    // For more information, see "common/components/free-camera-controller.hpp"
    class FreeCameraControllerSystem {
        Application *app;          // The application in which the state runs
        bool mouse_locked = false; // Is the mouse locked

        float slideTime = 0; // The time of sliding
        our::JumpState jumpState = our::JumpState::GROUNDED; // The state of jumping
        our::SlideState slideState = our::SlideState::NORMAL; // The state of sliding

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application *app) {
            this->app = app;
        }

        // This should be called every frame to update all entities containing a FreeCameraControllerComponent
        void update(World *world, float deltaTime, our::MotionState &motionState, bool &isSlided) {
            // First of all, we search for an cameraEntity containing both a CameraComponent and a FreeCameraControllerComponent
            // As soon as we find one, we break
            CameraComponent *camera = nullptr;
            FreeCameraControllerComponent *controller = nullptr;
            for (auto entity: world->getEntities()) {
                camera = entity->getComponent<CameraComponent>();
                controller = entity->getComponent<FreeCameraControllerComponent>();
                if (camera && controller)
                    break;
            }

            // Get player component
            PlayerComponent *player = nullptr;
            for (auto entity: world->getEntities()) {
                player = entity->getComponent<PlayerComponent>();
                if (player)
                    break;
            }

            // If there is no cameraEntity with both a CameraComponent and a FreeCameraControllerComponent, we can do nothing so we return
            if (!(camera && controller && player))
                return;

            // Get the cameraEntity that we found via getOwner of camera (we could use controller->getOwner())
            Entity *cameraEntity = camera->getOwner();

            // Get the playerEntity that we found via getOwner of player (we could use controller->getOwner())
            Entity *playerEntity = player->getOwner();

            // We get a reference to the playerEntity's position and rotation
            glm::vec3 &position = cameraEntity->localTransform.position;
            glm::vec3 &rotation = cameraEntity->localTransform.rotation;

            // We get a reference to the playerEntity's position
            glm::vec3 &playerPosition = playerEntity->localTransform.position;
            glm::vec3 &playerRotation = playerEntity->localTransform.rotation;
            // We get a reference to the cameraEntity's position
            glm::vec3 &cameraPosition = cameraEntity->localTransform.position;

            // If the left mouse button is pressed, we lock and hide the mouse. This common in First Person Games.
            if (app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && !mouse_locked) {
                app->getMouse().lockMouse(app->getWindow());
                mouse_locked = true;
                // If the left mouse button is released, we unlock and unhide the mouse.
            } else if (!app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && mouse_locked) {
                app->getMouse().unlockMouse(app->getWindow());
                mouse_locked = false;
            }

            // We prevent the pitch from exceeding a certain angle from the XZ plane to prevent gimbal locks
            if (rotation.x < -glm::half_pi<float>() * 0.99f)
                rotation.x = -glm::half_pi<float>() * 0.99f;
            if (rotation.x > glm::half_pi<float>() * 0.99f)
                rotation.x = glm::half_pi<float>() * 0.99f;

            // This is not necessary, but whenever the rotation goes outside the 0 to 2*PI range, we wrap it back inside.
            // This could prevent floating point error if the player rotates in single direction for an extremely long time.
            rotation.y = glm::wrapAngle(rotation.y);

            // We update the camera fov based on the mouse wheel scrolling amount
            float fov = camera->fovY + app->getMouse().getScrollOffset().y * controller->fovSensitivity;
            fov = glm::clamp(fov, glm::pi<float>() * 0.01f,
                             glm::pi<float>() * 0.99f); // We keep the fov in the range 0.01*PI to 0.99*PI
            camera->fovY = fov;

            // We get the player model matrix (relative to its parent) to compute the playerFront, playerUp and playerRight directions
            glm::mat4 playerMatrix = playerEntity->localTransform.toMat4();

            // We get the player model matrix (relative to its parent) to compute the playerFront, playerUp and playerRight directions
            glm::vec3 playerFront = glm::vec3(playerMatrix * glm::vec4(0, 0, -1, 0)),
                    playerUp = glm::vec3(playerMatrix * glm::vec4(0, 1, 0, 0)),
                    playerRight = glm::vec3(playerMatrix * glm::vec4(1, 0, 0, 0));

            // We get the camera model matrix (relative to its parent) to compute the cameraFront, cameraUp and cameraRight directions
            glm::mat4 cameraMatrix = cameraEntity->localTransform.toMat4();

            // We get the camera model matrix (relative to its parent) to compute the cameraFront, cameraUp and cameraRight directions
            glm::vec3 cameraFront = glm::vec3(cameraMatrix * glm::vec4(0, 0, -1, 0)),
                    cameraUp = glm::vec3(cameraMatrix * glm::vec4(0, 1, 0, 0)),
                    cameraRight = glm::vec3(cameraMatrix * glm::vec4(1, 0, 0, 0));

            // We get the current position sensitivity
            glm::vec3 current_sensitivity = controller->positionSensitivity;


            // We change the camera position based on the keys WASD/QE
            // S & W moves the player back and forth
            if (app->getKeyboard().isPressed(GLFW_KEY_W))
                position += cameraFront * (deltaTime * current_sensitivity.z);
            if (app->getKeyboard().isPressed(GLFW_KEY_S))
                position -= cameraFront * (deltaTime * current_sensitivity.z);

            // Q & E moves the player playerUp and down
            if (app->getKeyboard().isPressed(GLFW_KEY_Q))
                position += cameraUp * (deltaTime * current_sensitivity.y);

            if (app->getKeyboard().isPressed(GLFW_KEY_E))
                position += -cameraUp * (deltaTime * current_sensitivity.y);
            // A & D moves the player left or playerRight

            // Jump logic
            float jumpSpeed = 6;
            float jumpMaxHeight = 4;
            if ((app->getKeyboard().isPressed(GLFW_KEY_SPACE) || app->getKeyboard().isPressed(GLFW_KEY_UP)) &&
                app->levelState != 3) {
                if (jumpState == our::JumpState::GROUNDED && slideState == our::SlideState::NORMAL) {
#ifdef USE_SOUND
                    // We play the jump sound
                    irrklang::ISoundEngine *soundEngine = irrklang::createIrrKlangDevice();
                    soundEngine->play2D("audio/jump.mp3");
#endif
                    // We set the jump state to JUMPING
                    jumpState = our::JumpState::JUMPING;
                    // Start the jump
                    position.y += (deltaTime * jumpSpeed);
                }
            }
            // If the player jumps higher than the max height, we set the jump state to FALLING
            if (position.y >= jumpMaxHeight) {
                jumpState = our::JumpState::FALLING;
            } else if (position.y <= 1) {
                // If the player was falling, we set the jump state to GROUNDED
                if (jumpState == our::JumpState::FALLING) {
#ifdef USE_SOUND
                    // We play the jump sound
                    irrklang::ISoundEngine *soundEngine = irrklang::createIrrKlangDevice();
                    soundEngine->play2D("audio/jumpLand.mp3");
#endif
                }
                jumpState = our::JumpState::GROUNDED;
            }

            // We update the player position based on the jump state
            if (jumpState == our::JumpState::JUMPING) {
                position.y += (deltaTime * jumpSpeed);
            } else if (jumpState == our::JumpState::FALLING) {
                // We update the player position based on the jump state
                position.y -= (deltaTime * jumpSpeed);
            } else {
                // We make sure the player is grounded
                position.y = 1;
            }

            // slide logic
            if ((app->getKeyboard().isPressed(GLFW_KEY_S) || app->getKeyboard().isPressed(GLFW_KEY_DOWN)) &&
                app->levelState != 3) {
                if (slideState == our::SlideState::NORMAL && jumpState == our::JumpState::GROUNDED) {
                    isSlided = true;
                    slideState = our::SlideState::Slided;
#ifdef USE_SOUND
                    // We play the slide sound
                    irrklang::ISoundEngine *soundEngine = irrklang::createIrrKlangDevice();
                    if (soundEngine->isCurrentlyPlaying("audio/sliding.mp3"))
                        soundEngine->stopAllSounds();
                    soundEngine->play2D("audio/sliding.mp3");
#endif
                    playerRotation.x -= 90;
                    playerPosition.z -= 1;
                    playerPosition.y += 1;
                    slideTime = 0;
                }
            }
            if (slideState == our::SlideState::Slided) {
                slideTime += deltaTime;
                isSlided = true;
                if (slideTime >= deltaTime * 50) {
                    isSlided = false;
                    slideState = our::SlideState::NORMAL;

                    glm::vec4 actualposition = playerEntity->getLocalToWorldMatrix() *
                                               glm::vec4(playerEntity->localTransform.position, 1.0);
                    playerPosition.y -= 1;
                    playerPosition.z += 1;
                    playerRotation.x += 90;
                }
            }

            // Start running on enter press
            if (app->getKeyboard().isPressed(GLFW_KEY_ENTER)) {
                motionState = our::MotionState::RUNNING;
            }
            // Move player forward
            if (motionState == our::MotionState::RUNNING)
                position += cameraFront * (deltaTime * 20);
            // if (jumpState == our::JumpState::GROUNDED && slideState == our::SlideState::NORMAL) {
            // Move player left and right
            if (app->getKeyboard().isPressed(GLFW_KEY_D) || app->getKeyboard().isPressed(GLFW_KEY_RIGHT)) {
                // Stop player from going off the street
                if (world->level == 3) {
                    if (cameraPosition.z < 5)
                        cameraPosition -= cameraRight * (deltaTime * player->speed);
                } else {
                    if (cameraPosition.z > -5)
                        cameraPosition += cameraRight * (deltaTime * player->speed);
                }
            }
            if (app->getKeyboard().isPressed(GLFW_KEY_A) || app->getKeyboard().isPressed(GLFW_KEY_LEFT)) {
                // Stop player from going off the street
                if (world->level == 3) {
                    if (cameraPosition.z > -5)
                        cameraPosition += cameraRight * (deltaTime * player->speed);
                } else {
                    if (cameraPosition.z < 5)
                        cameraPosition -= cameraRight * (deltaTime * player->speed);
                }
            }
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit() {
            if (mouse_locked) {
                mouse_locked = false;
                app->getMouse().unlockMouse(app->getWindow());
            }
        }
    };

}
