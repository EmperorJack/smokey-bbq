#ifndef SMOKESIMULATION_H
#define SMOKESIMULATION_H

class SmokeSimulation {

    public:
        SmokeSimulation(float);

        void setupFields();
        void update();

        glm::vec2 advectedVelocityAt(int, int);
        float advectedDensityAt(int, int);
        glm::vec2 traceParticle(float, float, float);

        float getDensity(float, float);
        float getInterpolatedValueDensity(float, float);
        float getCellDensity(int, int);

        glm::vec2 getVelocity(float, float);
        float getInterpolatedValue(float, float, int);
        glm::vec2 getCellVelocity(int, int);

        void addPulse(glm::vec2);

        void render(glm::mat4, glm::vec2);
        void drawSquare(glm::mat4, bool);
        void drawLine(glm::mat4);

        float myRandom();
};

#endif
