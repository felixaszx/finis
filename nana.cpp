#include <thread>
#include <future>
#include <chrono>
#include <iostream>

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>

int main(int argc, char** argv)
{
    std::atomic_bool nana_running = true;
    std::thread nana_th(
        [&]()
        {
            nana::form fm({}, {800, 600});
            nana::button btn(fm, nana::rectangle{0, 0, 200, 200});
            btn.caption("hello nana");
            btn.events().click.connect([]() { std::cout << 1; });
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