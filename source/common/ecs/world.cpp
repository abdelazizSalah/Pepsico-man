#include "world.hpp"
#include "../deserialize-utils.hpp"
#include <vector>
#include <iostream>

namespace our {
    // This is the upper limit on the number of entities in the world
    const uint64_t ENTITIES_UPPER_LIMIT = 400;
    const uint8_t SLICE_SIZE = 13 ;

    // This is the map that keeps track of entities positions in the world
    // We have three ways left, middle and right
    std::vector<std::vector<bool>> entityMap(ENTITIES_UPPER_LIMIT, std::vector<bool>(7, false));

    int generateRandomNumber(int min, int max) {
        // Generate a random number between min and max (inclusive)
        int randomNumber = rand() % (max - min + 1) + min;
        return randomNumber;
    }

    // This will deserialize a json array of entities and add the new entities to the current world
    // If parent pointer is not null, the new entities will be have their parent set to that given pointer
    // If any of the entities has children, this function will be called recursively for these children
    void World::deserialize(const nlohmann::json &data, Entity *parent) {
        if (!data.is_array())
            return;
        for (const auto &entityData: data) {
            //(Req 8) Create an entity, make its parent "parent" and call its deserialize with "entityData".
            Entity *newEntity = add();           // create a new entity using the add function in world.hpp
            newEntity->parent = parent;          // set the parent of the new entity to the given parent
            newEntity->deserialize(entityData);  // deserialize the new entity using the given entityData
            if (entityData.contains("children")) // check if the entity has children
            {
                //(Req 8) Recursively call this world's "deserialize" using the children data
                //  and the current entity as the parent
                this->deserialize(entityData["children"], newEntity); // deserialize the children of the current entity
            }
            if (entityData.contains("duplicates")) {
                glm::vec3 duplicates = glm::vec3(entityData.value("duplicates", duplicates));
                for (int i = 1; i < (int) duplicates[0]; ++i) {
                    Entity *newDuplicateEntity = add();           // create a new entity using the add function in world.hpp
                    newDuplicateEntity->parent = parent;          // set the parent of the new entity to the given parent
                    newDuplicateEntity->deserialize(
                            entityData);  // deserialize the new entity using the given entityData
                    if ((bool) duplicates[2]) {
                        int horizontal = generateRandomNumber(0, 6);
                        int vertical = generateRandomNumber(0, ENTITIES_UPPER_LIMIT - 1);
                        while (entityMap[vertical][horizontal]) {
                            horizontal = generateRandomNumber(0, 6);
                            vertical = generateRandomNumber(0, ENTITIES_UPPER_LIMIT - 1);
                        }
                        entityMap[vertical][horizontal] = true;
                        newDuplicateEntity->localTransform.position.x += (-float(vertical)) * SLICE_SIZE;
                        newDuplicateEntity->localTransform.position.z = (-7.5f + float(horizontal) * 2.5f);
                        std::cout << newDuplicateEntity->localTransform.position.z << std::endl;
                    } else {
                        newDuplicateEntity->localTransform.position.x += -float(i) * duplicates[1];
                    }
                }

            }
        }
    }

}