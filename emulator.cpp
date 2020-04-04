#include "emulator.h"
#include <random>
#include <unordered_map>
#include <vector>

namespace {
    class TSimpleEmulator : public IEmulator {
    public:
        explicit TSimpleEmulator(const TContext& context)
            : Context(context)
            , RandomGenerator(std::random_device()())
        {
            const auto currentTime = Context.Clock.GetTime();
            SetNextBookingTime(currentTime);
        }

        void AddObserver(IEmulatorObserver& observer) override {
            Observers.push_back(&observer);
        }

        void MakeStep() override {
            const auto currentTime = Context.Clock.GetTime();
            if (currentTime.Day > 0) {
                HandleCheckoutActions(currentTime.Day);
            }
            HandleCheckinActions(currentTime.Day);
            GenerateBookings(currentTime);
        }

    private:
        void HandleCheckinActions(unsigned currentDay) {
            Checkins[currentDay];
            for (const auto& booking : Checkins.at(currentDay)) {
                const auto success = Context.BookingSystem.CheckInto(booking);
                ObserveCheckin(booking, success);
            }
            Checkins[currentDay].clear();
        }

        void HandleCheckoutActions(unsigned currentDay) {
            Checkouts[currentDay];
            for (const auto& booking : Checkouts.at(currentDay)) {
                const auto cost = Context.BookingSystem.GetBill(booking);
                ObserveCheckout(booking, cost);
            }
            Checkouts[currentDay].clear();
        }

        void GenerateBookings(IClock::TTime currentTime) {
            if (IsTimeToBook(currentTime)) {
                TBooking booking;
                booking.UserId = UserId;
                ++UserId;
                booking.DayFrom = currentTime.Day + DaysUntilBookingDistribution(RandomGenerator);
                booking.DayTo = booking.DayFrom + BookingDurationDistribution(RandomGenerator) - 1;
                booking.RoomType = GenerateRoomType();
                const auto success = Context.BookingSystem.Book(booking);
                ObserveBook(booking, success);
                if (success) {
                    Checkins[booking.DayFrom].push_back(booking);
                    Checkouts[booking.DayTo + 1].push_back(booking);
                }
                SetNextBookingTime(currentTime);
            }
        }

        void ObserveBook(const TBooking& booking, bool success) {
            for (auto* observer : Observers) {
                observer->OnBook(booking, success);
            }
        }

        void ObserveCheckin(const TBooking& booking, bool success) {
            for (auto* observer : Observers) {
                observer->OnCheckin(booking, success);
            }
        }

        void ObserveCheckout(const TBooking& booking, TCost cost) {
            for (auto* observer : Observers) {
                observer->OnCheckout(booking, cost);
            }
        }

        void SetNextBookingTime(IClock::TTime currentTime) {
            const auto durationUntilNextBooking = DistributionOfIntervalBetweenBookings(RandomGenerator);
            NextBookingTime = SumTime(currentTime, IClock::TTime{0, durationUntilNextBooking});
        }

        bool IsTimeToBook(IClock::TTime currentTime) const {
            auto timeAsTuple = [](IClock::TTime t) { return std::make_tuple(t.Day, t.Hour); };
            return timeAsTuple(currentTime) > timeAsTuple(NextBookingTime);
        }

        static IClock::TTime SumTime(IClock::TTime l, IClock::TTime r) {
            static constexpr unsigned HOURS_IN_DAY = 24;
            return {l.Day + r.Day + (l.Hour + r.Hour) / HOURS_IN_DAY, (l.Hour + r.Hour) % HOURS_IN_DAY};
        }

        ERoomType GenerateRoomType() {
            return static_cast<ERoomType>(RoomTypeDistribution(RandomGenerator));
        }

    private:
        const TContext Context;
        std::vector<IEmulatorObserver*> Observers;
        std::unordered_map<unsigned, std::vector<TBooking>> Checkins;
        std::unordered_map<unsigned, std::vector<TBooking>> Checkouts;
        std::mt19937 RandomGenerator;
        IClock::TTime NextBookingTime;
        std::uniform_int_distribution<unsigned> DistributionOfIntervalBetweenBookings{1, 5};
        std::uniform_int_distribution<unsigned> DaysUntilBookingDistribution{1, 10};
        std::uniform_int_distribution<unsigned> BookingDurationDistribution{1, 10};
        std::uniform_int_distribution<int> RoomTypeDistribution{0, static_cast<int>(ROOM_TYPES.size()) - 1};
        TUserId UserId = 0;
    };
}

std::unique_ptr<IEmulator> IEmulator::Create(const IEmulator::TContext& context) {
    return std::make_unique<TSimpleEmulator>(context);
}
