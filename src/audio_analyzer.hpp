#ifndef AUDIO_ANALYZER_H
#define AUDIO_ANALYZER_H

class AudioAnalyzer {

    public:

    AudioAnalyzer();

    void render(glm::mat4);
    void drawSquare(glm::mat4, bool);

    void shutDown();

};

#endif
