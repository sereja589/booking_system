#include "booking_system.h"
#include "clock.h"
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace {
    class THotelPlan {
    public:
        explicit THotelPlan(TRoomCounts roomCounts)
            : RoomCounts(std::move(roomCounts))
        {
            // TODO Один раз задать список enum-а
            if (!RoomCounts.count(ERoomType::Single)) {
                throw std::runtime_error("Single room count is not set");
            }
            if (!RoomCounts.count(ERoomType::Double)) {
                throw std::runtime_error("Double room count is not set");
            }
            BusyRooms[ERoomType::Single];
            BusyRooms[ERoomType::Double];
        }

        bool Has(ERoomType roomType, unsigned dayFrom, unsigned dayTo) const {
            if (RoomCounts.at(roomType) == 0) {
                return false;
            }
            for (unsigned day = dayFrom; day <= dayTo; ++day) {
                if (!BusyRooms.at(roomType).count(day)) {
                    continue;
                }
                if (BusyRooms.at(roomType).at(day).size() == RoomCounts.at(roomType)) {
                    return false;
                }
            }
            return true;
        }

        void Book(TUserId userId, ERoomType roomType, unsigned dayFrom, unsigned dayTo) {
            for (unsigned day = dayFrom; day <= dayTo; ++day) {
                BusyRooms[roomType][day];
                if (BusyRooms.at(roomType).at(day).size() == RoomCounts.at(roomType)) {
                    throw std::runtime_error(
                        "All rooms of type " + std::to_string(static_cast<int>(roomType)) +
                        " are busy at " + std::to_string(day) + " day"
                    );
                }
                BusyRooms[roomType][day].insert(userId);
            }
        }

        bool HasBooking(TUserId userId, ERoomType roomType, unsigned dayFrom, unsigned dayTo) const {
            for (unsigned day = dayFrom; day <= dayTo; ++day) {
                if (!BusyRooms.at(roomType).count(day)) {
                    return false;
                }
                if (BusyRooms.at(roomType).at(day).count(userId) == 0) {
                    return false;
                }
            }
            return true;
        }

    private:
        const TRoomCounts RoomCounts;
        // room type, date, busy rooms count
        std::unordered_map<ERoomType, std::unordered_map<unsigned, std::unordered_set<TUserId>>> BusyRooms;
    };

    class TTrivialBookingSystem : public IBookingSystem {
    public:
        TTrivialBookingSystem(TRoomCounts roomCounts, TRoomCosts roomCosts, const IClock& clock)
            : HotelPlan(std::move(roomCounts))
            , RoomCosts(std::move(roomCosts))
            , Clock(clock)
        {
        }

        bool Book(const TBooking& booking) override {
            if (HotelPlan.Has(booking.RoomType, booking.DayFrom, booking.DayTo)) {
                HotelPlan.Book(booking.UserId, booking.RoomType, booking.DayFrom, booking.DayTo);
                return true;
            }
            return false;
        }

        bool CheckInto(const TBooking& booking) override {
            const auto dayCount = booking.DayTo - booking.DayFrom + 1;
            if (dayCount == 0) {
                throw std::runtime_error("dayCount should be great than 0");
            }
            const auto currentDate = Clock.GetTime().Day;
            if (currentDate < booking.DayFrom) {
                throw std::runtime_error("booking starts on " + std::to_string(currentDate)
                    + " but now is " + std::to_string(currentDate));
            }
            if (HotelPlan.HasBooking(booking.UserId, booking.RoomType, currentDate, booking.DayTo)) {
                return true;
            }
            return false;
        }

        TCost GetBill(const TBooking &booking) override {
            return RoomCosts.at(booking.RoomType);
        }

    private:
        THotelPlan HotelPlan;
        TRoomCosts RoomCosts;
        const IClock& Clock;
    };
}

std::string RoomTypeToString(ERoomType roomType) {
    switch (roomType) {
        case ERoomType::Single:
            return "single";
        case ERoomType::Double:
            return "double";
    }
}

std::unique_ptr<IBookingSystem> IBookingSystem::Create(
    TRoomCounts roomCounts,
    TRoomCosts roomCosts,
    IBookingSystem::EType type,
    const IClock& clock
) {
    switch (type) {
        case IBookingSystem::EType::Trivial:
            return std::make_unique<TTrivialBookingSystem>(std::move(roomCounts), std::move(roomCosts), clock);
    }
}
