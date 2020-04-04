#pragma once

#include "clock.h"
#include <array>
#include <memory>
#include <unordered_map>

#define ROOMS \
    X(Single) \
    X(Double)

enum class ERoomType {
#define X(Id) Id,
    ROOMS
#undef X
};

constexpr std::array ROOM_TYPES {
#define X(Id) ERoomType::Id,
    ROOMS
#undef X
};

std::string RoomTypeToString(ERoomType roomType);

using TRoomCounts = std::unordered_map<ERoomType, unsigned>;
using TCost = unsigned;
using TRoomCosts = std::unordered_map<ERoomType, TCost>;
using TUserId = unsigned;

struct TBooking {
    TUserId UserId;
    ERoomType RoomType;
    unsigned DayFrom;
    unsigned DayTo;
};

using TDateGetter = std::function<unsigned()>;

// Отвечает за стратегию бронироования номеров
class IBookingSystem {
public:
    enum class EType {
        Trivial
    };

public:
    virtual ~IBookingSystem() = default;

    virtual bool Book(const TBooking& booking) = 0;
    virtual bool CheckInto(const TBooking& booking) = 0;
    virtual TCost GetBill(const TBooking& booking) = 0;

public:
    static std::unique_ptr<IBookingSystem> Create(TRoomCounts roomCounts, TRoomCosts roomCosts, EType type, const IClock& clock);
};
