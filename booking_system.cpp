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
            for (const auto roomType : ROOM_TYPES) {
                if (!RoomCounts.count(roomType)) {
                    throw std::runtime_error("Room count is not set for type " + RoomTypeToString(roomType));
                }
            }
            for (const auto roomType : ROOM_TYPES) {
                BusyRooms[roomType];
            }
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
            if (booking.DayTo < booking.DayFrom) {
                return false;
            }
            const auto currentDate = Clock.GetTime().Day;
            if (currentDate < booking.DayFrom || currentDate > booking.DayTo) {
                return false;
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

    std::vector<ERoomType> GetSuitableRoomTypes(ERoomType roomType) {
        auto it = std::find(std::begin(ROOM_TYPES), std::end(ROOM_TYPES), roomType);
        if (it == std::end(ROOM_TYPES)) {
            throw std::runtime_error("Invalid value of enum ERoomType");
        }
        std::vector<ERoomType> result;
        while (it != std::end(ROOM_TYPES)) {
            result.push_back(*it);
            ++it;
        }
        return result;
    }

    class TSmartBookingSystem : public IBookingSystem {
    public:
        TSmartBookingSystem(TRoomCounts roomCounts, TRoomCosts roomCosts, const IClock& clock)
            : HotelPlan(std::move(roomCounts))
            , RoomCosts(std::move(roomCosts))
            , Clock(clock)
        {
        }

        bool Book(const TBooking& booking) override {
            const auto suitableRoomTypes = GetSuitableRoomTypes(booking.RoomType);
            for (const auto roomType : suitableRoomTypes) {
                if (HotelPlan.Has(roomType, booking.DayFrom, booking.DayTo)) {
                    HotelPlan.Book(booking.UserId, roomType, booking.DayFrom, booking.DayTo);
                    return true;
                }
            }
            return false;
        }

        bool CheckInto(const TBooking& booking) override {
            if (booking.DayTo < booking.DayFrom) {
                return false;
            }
            const auto currentDate = Clock.GetTime().Day;
            if (currentDate < booking.DayFrom || currentDate > booking.DayTo) {
                return false;
            }

            const auto suitableRoomTypes = GetSuitableRoomTypes(booking.RoomType);
            auto hasBooking = [this, &booking, currentDate](ERoomType roomType) {
                return HotelPlan.HasBooking(booking.UserId, roomType, currentDate, booking.DayTo);
            };
            return std::any_of(std::begin(suitableRoomTypes), std::end(suitableRoomTypes), hasBooking);

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
        #define X(Id) case ERoomType::Id: return #Id;
            ROOMS
        #undef X
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
        case IBookingSystem::EType::Smart:
            return std::make_unique<TSmartBookingSystem>(std::move(roomCounts), std::move(roomCosts), clock);
    }
}
