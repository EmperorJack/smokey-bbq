//
// Created by Jack Purvis
//

#ifndef SMOKEYBBQ_MANAGER_GUI_HPP
#define SMOKEYBBQ_MANAGER_GUI_HPP

#include <manager.hpp>

class ManagerGui {

public:

    // Setup
    ManagerGui(Manager* manager);

    // Rendering
    void render();

private:

    // Manager instance
    Manager* manager;

    // Composition selector
    int compositionSelect;

    // Rendering
    void renderToggles();
    void renderCompositionSelector();

};

#endif //SMOKEYBBQ_MANAGER_GUI_HPP
