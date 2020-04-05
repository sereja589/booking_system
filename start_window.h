#pragma once

#include <QLineEdit>
#include <QMainWindow>
#include <QTimer>
#include "emulator.h"
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class TStartWindow; }
QT_END_NAMESPACE

class TClock : public IClock {
public:
    TTime GetTime() const override;

    void Add(unsigned additionalHours);

private:
    std::atomic_uint Hours{0};
};

struct TBookingEvent {
    TBooking Booking;
    bool Success;
};

struct TCheckinEvent {
    TBooking Booking;
    bool Success;
};

struct TCheckoutEvent {
    TBooking Booking;
    TCost Cost;
};

class THotelStats {
public:
    THotelStats(const TRoomCounts& roomCounts)
        : RoomCounts(roomCounts)
    {
    }

    void AddAcceptedBooking() {
        ++AcceptedBookings;
    }

    void AddRejectedBooking() {
        ++RejectedBookings;
    }

    unsigned GetAcceptedBookings() const {
        return AcceptedBookings;
    }

    unsigned GetTotalBookings() const {
        return AcceptedBookings + RejectedBookings;
    }

    void AddRoomsOccupanccy(ERoomType roomType, unsigned day, double occupancy) {
        if (RoomsOccupancy[roomType].size() <= day) {
            RoomsOccupancy[roomType].resize(day + 1);
        }
        RoomsOccupancy[roomType][day] = occupancy;
    }

    double GetRoomOccupancy(ERoomType roomType) const {
        const auto& occupancy = RoomsOccupancy.at(roomType);
        return std::reduce(occupancy.begin(), occupancy.end()) / occupancy.size();
    }

    // TODO Можно ли так?
    double GetRoomOccupancy() const {
        double occupancy = 0;
        unsigned totalRoomCounts = 0;
        for (const auto roomType : ROOM_TYPES) {
            occupancy += RoomCounts.at(roomType) * GetRoomOccupancy(roomType);
            totalRoomCounts += RoomCounts.at(roomType);
        }
        return occupancy / totalRoomCounts;
    }

private:
    const TRoomCounts& RoomCounts;

    unsigned AcceptedBookings = 0;
    unsigned RejectedBookings = 0;
    std::unordered_map<ERoomType, std::vector<double>> RoomsOccupancy;
};

class TStartWindow : public QMainWindow, public IEmulatorObserver {
    Q_OBJECT

public:
    TStartWindow(QWidget *parent = nullptr);
    ~TStartWindow();

private slots:
    void on_StartEmulate_clicked();
    void on_MakeStep_clicked();
    void on_StopEmulation_clicked();
    void on_StartEmulationAutomatically_clicked();

private:
    unsigned GetEmulateStep() const;
    unsigned GetIntervalBetweenStep() const;
    unsigned GetDaysToEmulate() const;
    IBookingSystem::EType GetBookingSystemType() const;

    void InitRoomCounts();
    void InitRoomCosts();

    void OnBook(const TBooking& booking, bool success) override;
    void OnCheckin(const TBooking& booking, bool success) override;
    void OnCheckout(const TBooking& booking, TCost cost) override;

    void DisplayRoomCounts();
    void DisplayTime();
    void DisplayProfit();
    void ClearEvents();
    void DisplayLastEvents();
    void DisplayStat();

private:
    Ui::TStartWindow* ui;

    std::unordered_map<ERoomType, const QLineEdit*> RoomCountInputs;
    std::unordered_map<ERoomType, const QLineEdit*> RoomCostInputs;

    std::unordered_map<ERoomType, QLineEdit*> OutputsOfBusyRooms;
    std::unordered_map<ERoomType, QLineEdit*> OutputsOfFreeRooms;

    TRoomCosts RoomCosts;
    TRoomCounts RoomCounts;
    TRoomCounts BusyRooms;

    TCost TotalProfit = 0;

    std::vector<TBookingEvent> Bookings;
    std::vector<TCheckinEvent> Checkins;
    std::vector<TCheckoutEvent> Checkouts;

    std::unique_ptr<THotelStats> HotelStats;

    std::unique_ptr<TClock> Clock;
    std::unique_ptr<IBookingSystem> BookingSystem;
    std::unique_ptr<IEmulator> Emulator;
    QTimer EmulationTimer;
};
