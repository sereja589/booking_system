#pragma once

#include "booking_system.h"
#include "clock.h"

class IEmulatorObserver {
public:
    virtual ~IEmulatorObserver() = default;

    virtual void OnBook(const TBooking& booking, bool success) = 0;
    virtual void OnCheckin(const TBooking& booking, bool success) = 0;
    virtual void OnCheckout(const TBooking& booking, TCost cost) = 0;
};

// Отвечает за стратегию создания заказов
// и эмулирует заказы по этой стратегии
class IEmulator {
public:
    struct TContext {
        IBookingSystem& BookingSystem;
        const IClock& Clock;
    };

public:
    virtual ~IEmulator() = default;

    virtual void AddObserver(IEmulatorObserver& observer) = 0;
    virtual void MakeStep() = 0;

public:
    static std::unique_ptr<IEmulator> Create(const TContext& context);
};
