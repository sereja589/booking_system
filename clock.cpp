#include "clock.h"
#include <ctime>
#include <memory>
#include <mutex>
#include <thread>

namespace {
    constexpr unsigned SECONDS_IN_DAY = 3600 * 24;

    class TRealClock : public IClock {
    public:
        TRealClock()
            : StartTime(std::time(nullptr))
        {}

        TTime GetTime() const override {
            const auto seconds = std::time(nullptr) - StartTime;
            return {
                static_cast<unsigned>(seconds / SECONDS_IN_DAY),
                static_cast<unsigned>(seconds % SECONDS_IN_DAY)
            };
        }

        void Sleep(unsigned seconds) const override {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
        }

    private:
        time_t StartTime;
    };

    class TVirtualClock : public IClock {

    };

    std::unique_ptr<IClock> Clock;
}


const IClock& GetClock() {
    if (Clock) {
        return *Clock;
    }
    throw std::runtime_error("Clock is not initialized");
}

void SetClock(const TClockParams& params) {

}

void SetRealClock() {
    Clock = std::make_unique<TRealClock>();
}
