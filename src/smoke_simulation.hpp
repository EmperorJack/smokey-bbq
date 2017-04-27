#ifndef SMOKESIMULATION_H
#define SMOKESIMULATION_H

class SmokeSimulation {

    public:
        SmokeSimulation(float);

        void setupVelocityField();
        void update();

        glm::vec2 advectedVelocityAt(int, int);
        glm::vec2 traceParticle(float, float, float);

        glm::vec2 getVelocity(float, float);
        float getInterpolatedValue(float, float, int);
        glm::vec2 getCellVelocity(int, int);

        void render(glm::mat4, glm::vec2);
        void drawSquare(glm::mat4);
        void drawLine(glm::mat4);

        float myRandom();
};

#endif
