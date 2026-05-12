#include <view/UIController.hpp>
#include <iostream>

int main() {
    std::cout << "[INFO] Starting task scheduler...\n";
    view::UIController ui;
    ui.execute();
    std::cout << "[INFO] Task scheduler was finished.\n";
    return 0;
}