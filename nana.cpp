#include <thread>
#include <future>
#include <chrono>
#include <iostream>
#include <windows.h>

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/menubar.hpp>

int main(int argc, char** argv)
{
    SetProcessDPIAware();
    std::atomic_bool nana_running = true;
    std::thread nana_th(
        [&]()
        {
            nana::form fm({}, {800, 600});
            nana::menubar menubar(fm);
            menubar.push_back("File").append("New", [](nana::menu::item_proxy& item) { std::cout << item.text(); });

            fm.show();
            nana::exec();
            nana_running = false;
        });

    using namespace std::chrono_literals;
    while (std::this_thread::sleep_for(16ms), nana_running)
    {
    }
    nana_th.join();
    return EXIT_SUCCESS;
}