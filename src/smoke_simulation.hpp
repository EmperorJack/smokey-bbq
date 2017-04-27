#ifndef SMOKESIMULATION_H
#define SMOKESIMULATION_H

class SmokeSimulation {

    public:
        SmokeSimulation(float);
        void setupVelocityField();
        void update();
        void render(glm::mat4);
        void drawSquare(glm::mat4);
        void drawVector(glm::mat4);
        float myRandom();
};

#endif
