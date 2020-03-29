#pragma once

#include "clock.h"
#include <memory>
#include <unordered_map>

enum class ERoomType {
    Single,
    Lux,
    HalfLux,
    // ...
};

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
